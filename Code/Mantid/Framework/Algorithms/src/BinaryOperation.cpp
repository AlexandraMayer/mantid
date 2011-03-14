//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {
    BinaryOperation::BinaryOperation()
      : API::PairedGroupAlgorithm(),
      m_ClearRHSWorkspace(false),
      m_useHistogramForRhsEventWorkspace(false),
      m_do2D_even_for_SingleColumn_on_rhs(false),
      m_indicesToMask(),
      m_progress(NULL)
    {}
    
    BinaryOperation::~BinaryOperation()
    {
      if (m_progress) delete m_progress;
    }
    
    /** Initialisation method.
     *  Defines input and output workspaces
     *
     */
    void BinaryOperation::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName1(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName2(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputPropName(),"",Direction::Output));
      declareProperty(new PropertyWithValue<bool>("AllowDifferentNumberSpectra", false, Direction::Input),
          "Allow workspaces to have different number of spectra and perform\n"
          "operation on LHS spectra using matching detector IDs in RHS.\n");

      declareProperty(new PropertyWithValue<bool>("ClearRHSWorkspace", false, Direction::Input),
          "Clear the RHS workspace as the operation is performed.\n"
          "This can help prevent maxing out available memory for large workspaces.\n"
          "This is ignored unless the RHS workspace is an EventWorkpsace.\n");
    }


    //--------------------------------------------------------------------------------------------
    /** Executes the algorithm. Will call execEvent() if appropriate.
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
    void BinaryOperation::exec()
    {
      // get input workspace, dynamic cast not needed
      m_lhs = getProperty(inputPropName1());
      m_rhs = getProperty(inputPropName2());
      m_AllowDifferentNumberSpectra = getProperty("AllowDifferentNumberSpectra");

      // Cast to EventWorkspace pointers
      m_elhs = boost::dynamic_pointer_cast<const EventWorkspace>(m_lhs);
      m_erhs = boost::dynamic_pointer_cast<const EventWorkspace>(m_rhs);

      // We can clear the RHS workspace if it is an event,
      //  and we are not doing mismatched spectra (in which case you might clear it too soon!)
      //  and lhs is not rhs.
      //  and out is not rhs.
      m_ClearRHSWorkspace = getProperty("ClearRHSWorkspace");
      if (m_ClearRHSWorkspace)
      {
        if (m_AllowDifferentNumberSpectra || (!m_erhs) || (m_rhs == m_lhs) || (m_out == m_rhs))
        {
          //std::cout << "m_ClearRHSWorkspace = false\n";
          m_ClearRHSWorkspace = false;
        }
      }


      //Get the output workspace
      m_out = getProperty(outputPropName());
      m_eout = boost::dynamic_pointer_cast<EventWorkspace>(m_out);

      // Make a check of what will be needed to setup the workspaces, based on the input types.
      this->checkRequirements();

      if (m_flipSides)
      {
        //Flip the workspaces left and right
        MatrixWorkspace_const_sptr temp = m_lhs;
        m_lhs = m_rhs;
        m_rhs = temp;
        EventWorkspace_const_sptr etemp = m_elhs;
        m_elhs = m_erhs;
        m_erhs = etemp;
      }

      // Check that the input workspace are compatible
      if (!checkCompatibility(m_lhs,m_rhs))
      {
        std::ostringstream ostr;
        ostr << "The two workspaces are not compatible for algorithm " << this->name();
        g_log.error() << ostr.str() << std::endl;
        throw std::invalid_argument( ostr.str() );
      }


      //Is the output going to be an EventWorkspace?
      if (m_keepEventWorkspace)
      {
        //The output WILL be EventWorkspace (this implies lhs is EW or rhs is EW + it gets flipped)
        if (!m_elhs)
          throw std::runtime_error("BinaryOperation:: the output was set to be an EventWorkspace (m_keepEventWorkspace == true), but the lhs is not an EventWorkspace. There must be a mistake in the algorithm. Contact the developers.");

        if (m_out == m_lhs)
        {
          //Will be modifying the EventWorkspace in-place on the lhs. Good.
          if (!m_eout)
            throw std::runtime_error("BinaryOperation:: the output was set to be lhs, and to be an EventWorkspace (m_keepEventWorkspace == true), but the output is not an EventWorkspace. There must be a mistake in the algorithm. Contact the developers.");
        }
        else
        {
          //You HAVE to copy the data from lhs to to the output!

          //Create a copy of the lhs workspace
          m_eout = boost::dynamic_pointer_cast<EventWorkspace>(API::WorkspaceFactory::Instance().create("EventWorkspace", m_elhs->getNumberHistograms(), 2, 1));
          //Copy geometry, spectra map, etc. over.
          API::WorkspaceFactory::Instance().initializeFromParent(m_elhs, m_eout, false);
          //And we copy all the events from the lhs
          m_eout->copyDataFrom( *m_elhs );
          //Make sure m_out still points to the same as m_eout;
          m_out = boost::dynamic_pointer_cast<API::MatrixWorkspace>(m_eout);
        }

        //Always clear the MRUs.
        m_eout->clearMRU();
        if (m_elhs) m_elhs->clearMRU();
        if (m_erhs) m_erhs->clearMRU();

      }
      else
      {
        // ---- Output will be WS2D -------

        // We need to create a new workspace for the output if:
        //   (a) the output workspace hasn't been set to one of the input ones, or
        //   (b) output WAS == rhs, but it has been flipped for whatever reason (incorrect size)
        //        meaning that it now points to lhs (which we don't want to change)
        //        so we need to copy lhs into m_out.
        if ( (m_out != m_lhs && m_out != m_rhs) ||
            ( m_out == m_lhs && ( m_flipSides ) )  )
        {
          // Make sure to delete anything that might be in the output name.
          if (AnalysisDataService::Instance().doesExist(getPropertyValue(outputPropName() )))
            AnalysisDataService::Instance().remove(getPropertyValue(outputPropName() ));
          m_out = WorkspaceFactory::Instance().create(m_lhs);
        }
      }


      // only overridden for some operations (plus and minus at the time of writing)
      operateOnRun(m_lhs->run(), m_rhs->run(), m_out->mutableRun());

      // Initialise the progress reporting object
      m_progress = new Progress(this,0.0,1.0,m_lhs->getNumberHistograms());

      // There are now 4 possible scenarios, shown schematically here:
      // xxx x   xxx xxx   xxx xxx   xxx x
      // xxx   , xxx xxx , xxx     , xxx x
      // xxx   , xxx xxx   xxx       xxx x
      // So work out which one we have and call the appropriate function

      // Single value workspace on the right : if it is an EventWorkspace with 1 spectrum, 1 bin, it is treated as a scalar
      if ( (m_rhs->size() == 1) && !m_do2D_even_for_SingleColumn_on_rhs  )
      {
        doSingleValue(); //m_lhs,m_rhs,m_out
      }
      else if ( m_rhs->getNumberHistograms() == 1 ) // Single spectrum on rhs
      {
        doSingleSpectrum();
      }
      // Single column on rhs; if the RHS is an event workspace with one bin, it is treated as a scalar.
      else if ( (m_rhs->blocksize() == 1) && !m_do2D_even_for_SingleColumn_on_rhs  )
      {
        m_indicesToMask.reserve(m_out->getNumberHistograms());
        doSingleColumn();
      }
      else // The two are both 2D and should be the same size (except if LHS is an event workspace)
      {
        m_indicesToMask.reserve(m_out->getNumberHistograms());

        bool mismatchedSpectra =(m_AllowDifferentNumberSpectra && (m_rhs->getNumberHistograms() != m_lhs->getNumberHistograms()));
        do2D(mismatchedSpectra);
      }

      applyMaskingToOutput(m_out);
      setOutputUnits(m_lhs,m_rhs,m_out);

      //For EventWorkspaces, redo the spectra to detector ID to make sure it is up-to-date. This may only be necessary for the Plus algorithm!
      if (m_eout) m_eout->makeSpectraMap();


      // Assign the result to the output workspace property
      setProperty(outputPropName(),m_out);

      return;
    }


    //--------------------------------------------------------------------------------------------
    /**
     * Execute a binary operation on events. Should be overridden.
     * @param lhs :: left-hand event workspace
     * @param rhs :: right-hand event workspace
     */
    void BinaryOperation::execEvent( DataObjects::EventWorkspace_const_sptr lhs, DataObjects::EventWorkspace_const_sptr rhs )
    {
      (void) lhs;(void) rhs; //Avoid compiler warnings
      //This should never happen
      throw Exception::NotImplementedError("BinaryOperation::execEvent() is not implemented for this operation.");
    }



    //--------------------------------------------------------------------------------------------
    /**
     * Return true if the two workspaces are compatible for this operation
     * Virtual: will be overridden as needed.
     * @param lhs :: left-hand workspace to check
     * @param rhs :: right-hand workspace to check
     * @return flag for the compatibility to the two workspaces
     */
    bool BinaryOperation::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      Unit_const_sptr lhs_unit = Unit_sptr();
      Unit_const_sptr rhs_unit = Unit_sptr();
      if ( lhs->axes() && rhs->axes() ) // If one of these is a WorkspaceSingleValue then we don't want to check units match
      {
        lhs_unit = lhs->getAxis(0)->unit();
        rhs_unit = rhs->getAxis(0)->unit();
      }

      const std::string lhs_unitID = ( lhs_unit ? lhs_unit->unitID() : "" );
      const std::string rhs_unitID = ( rhs_unit ? rhs_unit->unitID() : "" );

      // Check the workspaces have the same units and distribution flag
      if ( lhs_unitID != rhs_unitID && lhs->blocksize() > 1 && rhs->blocksize() > 1 )
      {
        g_log.error("The two workspace are not compatible because they have different units on the X axis.");
        return false;
      }

      // Check the size compatibility
      if (!checkSizeCompatibility(lhs,rhs))
      {
        std::ostringstream ostr;
        ostr<<"The sizes of the two workspaces " <<
            "(" << lhs->getName() << ": " << lhs->getNumberHistograms() << " spectra, blocksize " << lhs->blocksize() << ")"
            << " and " <<
            "(" << rhs->getName() << ": " << rhs->getNumberHistograms() << " spectra, blocksize " << rhs->blocksize() << ")"
            << " are not compatible for algorithm "<<this->name();
        g_log.error() << ostr.str() << std::endl;
        throw std::invalid_argument( ostr.str() );
      }

      return true;
    }

    //--------------------------------------------------------------------------------------------
    /** Return true if the two workspaces can be treated as event workspaces
     * for the binary operation. If so, execEvent() will be called.
     * (e.g. Plus algorithm will concatenate event lists)
     * @param lhs :: left-hand event workspace to check
     * @param rhs :: right-hand event workspace to check
     * @return false by default; will be overridden by specific algorithms
     */
    bool BinaryOperation::checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs)
    {
      (void) lhs; (void) rhs; //Avoid compiler warning
      return false;
    }


    //--------------------------------------------------------------------------------------------
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     *  In order to be size compatible then the larger workspace
     *  must divide be the size of the smaller workspace leaving no remainder
     *  @param lhs :: the first workspace to compare
     *  @param rhs :: the second workspace to compare
     *  @retval true The two workspaces are size compatible
     *  @retval false The two workspaces are NOT size compatible
     */
    bool BinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      const int lhsSize = lhs->size();
      const int rhsSize = rhs->size();
      // A SingleValueWorkspace on the right matches anything
      if ( rhsSize == 1 ) return true;
      // The rhs must not be smaller than the lhs
      if ( lhsSize < rhsSize ) return false;

      //Did checkRequirements() tell us that the X histogram size did not matter?
      if (!m_matchXSize)
        //If so, only the vertical # needs to match
        return (lhs->getNumberHistograms() == rhs->getNumberHistograms());

      // Otherwise they must match both ways, or horizontally or vertically with the other rhs dimension=1
      if ( rhs->blocksize() == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms() ) return true;
      // Past this point, we require the X arrays to match. Note this only checks the first spectrum
      if ( !WorkspaceHelpers::matchingBins(lhs,rhs,true) ) return false;
      
      const int rhsSpec = rhs->getNumberHistograms();

      return ( lhs->blocksize() == rhs->blocksize() && ( rhsSpec==1 || lhs->getNumberHistograms() == rhsSpec ) );
    }


    //--------------------------------------------------------------------------------------------
    /**
     * Checks if the spectra at the given index of either input workspace is masked. If so then the output spectra has zeroed data
     * and is also masked. 
     * @param lhs :: A pointer to the left-hand operand
     * @param rhs :: A pointer to the right-hand operand
     * @param index :: The workspace index to check
     * @param out :: A pointer to the output workspace
     * @returns True if further processing is not required on the spectra, false if the binary operation should be performed.
     */
    bool BinaryOperation::propagateSpectraMask(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs, 
        const int index, API::MatrixWorkspace_sptr out)
    {
      bool continueOp(true);
      IDetector_sptr det_lhs, det_rhs;
      try
      {
        det_lhs = lhs->getDetector(index);
        det_rhs = rhs->getDetector(index);
      }
      catch(std::runtime_error &)
      {
      }
      catch(std::domain_error &)
      {
        // try statement will throw a domain_error when the axis is not a spectra axis.
        return continueOp;
      }
      if( (det_lhs && det_lhs->isMasked()) || ( det_rhs && det_rhs->isMasked()) )
      {
        continueOp = false;
        //Zero the output data and ensure that the output spectra is masked. The masking is done outside of this
        //loop modiying the parameter map in a multithreaded loop requires too much locking
        m_indicesToMask.push_back(index);
        MantidVec & yValues = out->dataY(index);
        MantidVec & eValues = out->dataE(index);
        MantidVec::const_iterator yend = yValues.end();
        for( MantidVec::iterator yit(yValues.begin()), eit(eValues.begin()); yit != yend; ++yit, ++eit)
        {
          (*yit) = 0.0;
          (*eit) = 0.0;
        }
      }
      return continueOp;
    }

    //--------------------------------------------------------------------------------------------
    /**
     * Called when the rhs operand is a single value.
     *  Loops over the lhs workspace calling the abstract binary operation function with a single number as the rhs operand.
     */
    void BinaryOperation::doSingleValue()
    {
      // Don't propate masking from the rhs here - it would be decidedly odd if the single value was masked

      // Pull out the single value and its error
      const double rhsY = m_rhs->readY(0)[0];
      const double rhsE = m_rhs->readE(0)[0];
      
      // Now loop over the spectra of the left hand side calling the virtual function
      const int numHists = m_lhs->getNumberHistograms();

      if (m_eout)
      {
        // ---- The output is an EventWorkspace ------
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          m_out->setX(i, m_lhs->refX(i));
          performEventBinaryOperation(m_eout->getEventList(i), rhsY, rhsE);
          m_progress->report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }
      else
      {
        // ---- Histogram Output -----
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          m_out->setX(i,m_lhs->refX(i));
          performBinaryOperation(m_lhs->readX(i),m_lhs->readY(i),m_lhs->readE(i),rhsY,rhsE,m_out->dataY(i),m_out->dataE(i));
          m_progress->report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }

   }
   

    //--------------------------------------------------------------------------------------------
    /** Called when the m_rhs operand is a 2D workspace of single values.
     *  Loops over the workspaces calling the abstract binary operation function with a single number as the m_rhs operand.
     */
    void BinaryOperation::doSingleColumn()
    {
      // Don't propate masking from the m_rhs here - it would be decidedly odd if the single bin was masked

      // Now loop over the spectra of the left hand side pulling m_out the single value from each m_rhs 'spectrum'
      // and then calling the virtual function
      const int numHists = m_lhs->getNumberHistograms();

      if (m_eout)
      {
        // ---- The output is an EventWorkspace ------
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          const double rhsY = m_rhs->readY(i)[0];
          const double rhsE = m_rhs->readE(i)[0];

          //m_out->setX(i, m_lhs->refX(i)); //unnecessary - that was copied before.
          if( propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
          {
            performEventBinaryOperation(m_eout->getEventList(i), rhsY, rhsE);
          }
          m_progress->report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }
      else
      {
        // ---- Histogram Output -----
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          const double rhsY = m_rhs->readY(i)[0];
          const double rhsE = m_rhs->readE(i)[0];

          m_out->setX(i,m_lhs->refX(i));
          if( propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
          {
            performBinaryOperation(m_lhs->readX(i),m_lhs->readY(i),m_lhs->readE(i),rhsY,rhsE,m_out->dataY(i),m_out->dataE(i));
          }
          m_progress->report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }


    }

    

    //--------------------------------------------------------------------------------------------
    /** Called when the m_rhs operand is a single spectrum.
     *  Loops over the lhs workspace calling the abstract binary operation function.
     */
    void BinaryOperation::doSingleSpectrum()
    {

      // Propagate any masking first or it could mess up the numbers
      //TODO: Check if this works for event workspaces...
      propagateBinMasks(m_rhs,m_out);


      if (m_eout)
      {
        // ----------- The output is an EventWorkspace -------------

        if (m_erhs)
        {
          // -------- The rhs is ALSO an EventWorkspace --------

          // Pull out the single eventList on the right
          const EventList & rhs_spectrum = m_erhs->getEventList(0);

          // Now loop over the spectra of the left hand side calling the virtual function
          const int numHists = m_lhs->getNumberHistograms();
          PARALLEL_FOR3(m_lhs,m_rhs,m_out)
          for (int i = 0; i < numHists; ++i)
          {
            PARALLEL_START_INTERUPT_REGION
            //m_out->setX(i,m_lhs->refX(i)); //unnecessary - that was copied before.

            //Perform the operation on the event list on the output (== lhs)
            performEventBinaryOperation(m_eout->getEventList(i), rhs_spectrum);
            m_progress->report();
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
        }
        else
        {
          // -------- The rhs is a histogram ---------
          // Pull m_out the m_rhs spectrum
          const MantidVec& rhsX = m_rhs->readX(0);
          const MantidVec& rhsY = m_rhs->readY(0);
          const MantidVec& rhsE = m_rhs->readE(0);

          // Now loop over the spectra of the left hand side calling the virtual function
          const int numHists = m_lhs->getNumberHistograms();
          PARALLEL_FOR3(m_lhs,m_rhs,m_out)
          for (int i = 0; i < numHists; ++i)
          {
            PARALLEL_START_INTERUPT_REGION
            //m_out->setX(i,m_lhs->refX(i)); //unnecessary - that was copied before.
            //Perform the operation on the event list on the output (== lhs)
            performEventBinaryOperation(m_eout->getEventList(i), rhsX, rhsY, rhsE);
            m_progress->report();
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
        }

      }
      else
      {
        // -------- The output is a histogram ----------
        // (inputs can be EventWorkspaces, but their histogram representation
        //  will be used instead)

        // Pull m_out the m_rhs spectrum
        const MantidVec& rhsY = m_rhs->readY(0);
        const MantidVec& rhsE = m_rhs->readE(0);

        // Now loop over the spectra of the left hand side calling the virtual function
        const int numHists = m_lhs->getNumberHistograms();
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          m_out->setX(i,m_lhs->refX(i));
          performBinaryOperation(m_lhs->readX(i),m_lhs->readY(i),m_lhs->readE(i),rhsY,rhsE,m_out->dataY(i),m_out->dataE(i));
          m_progress->report();
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }
    }


    //--------------------------------------------------------------------------------------------
    /** Called when the two workspaces are the same size. 
     *  Loops over the workspaces extracting the appropriate spectra and calling the abstract binary operation function.
     *
     *  @param mismatchedSpectra :: allow the # of spectra to not be the same. Will use the
     *      detector IDs to find the corresponding spectrum on RHS
     */
    void BinaryOperation::do2D( bool mismatchedSpectra)
    {
      BinaryOperationTable * table = NULL;
      if (mismatchedSpectra)
        table = BinaryOperation::buildBinaryOperationTable(m_lhs, m_rhs);


      // Propagate any masking first or it could mess up the numbers
      //TODO: Check if this works for event workspaces...
      propagateBinMasks(m_rhs,m_out);


      if (m_eout)
      {
        // ----------- The output is an EventWorkspace -------------

        if (m_erhs && !m_useHistogramForRhsEventWorkspace)
        {
           // ------------ The rhs is ALSO an EventWorkspace ---------------
          // Now loop over the spectra of each one calling the virtual function
          const int numHists = m_lhs->getNumberHistograms();
          PARALLEL_FOR3(m_lhs,m_rhs,m_out)
          for (int i = 0; i < numHists; ++i)
          {
            PARALLEL_START_INTERUPT_REGION
            m_progress->report();

            int rhs_wi = i;
            if (mismatchedSpectra && table)
            {
              rhs_wi = (*table)[i];
              if (rhs_wi < 0)
                continue;
            }
            else
            {
              // Check for masking except when mismatched sizes
              if(!propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
                continue;
            }
            //Reach here? Do the division
            //Perform the operation on the event list on the output (== lhs)
            performEventBinaryOperation(m_eout->getEventList(i),  m_erhs->getEventList(rhs_wi));

            //Free up memory on the RHS if that is possible
            if (m_ClearRHSWorkspace)
              const_cast<EventList&>(m_erhs->getEventList(rhs_wi)).clear();
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
        }
        else
        {
          // -------- The rhs is a histogram, or we want to use the histogram representation of it ---------

          // Now loop over the spectra of each one calling the virtual function
          const int numHists = m_lhs->getNumberHistograms();
          PARALLEL_FOR3(m_lhs,m_rhs,m_out)
          for (int i = 0; i < numHists; ++i)
          {
            PARALLEL_START_INTERUPT_REGION
            m_progress->report();
            int rhs_wi = i;
            if (mismatchedSpectra && table)
            {
              rhs_wi = (*table)[i];
              if (rhs_wi < 0)
                continue;
            }
            else
            {
              // Check for masking except when mismatched sizes
              if(!propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
                continue;
            }

            //Reach here? Do the division
            performEventBinaryOperation(m_eout->getEventList(i),  m_rhs->readX(rhs_wi), m_rhs->readY(rhs_wi), m_rhs->readE(rhs_wi));

            //Free up memory on the RHS if that is possible
            if (m_ClearRHSWorkspace)
              const_cast<EventList&>(m_erhs->getEventList(rhs_wi)).clear();

            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
        }

      }
      else
      {
        // -------- The output is a histogram ----------
        // (inputs can be EventWorkspaces, but their histogram representation
        //  will be used instead)

        // Now loop over the spectra of each one calling the virtual function
        const int numHists = m_lhs->getNumberHistograms();
        PARALLEL_FOR3(m_lhs,m_rhs,m_out)
        for (int i = 0; i < numHists; ++i)
        {
          PARALLEL_START_INTERUPT_REGION
          m_progress->report();
          m_out->setX(i,m_lhs->refX(i));
          int rhs_wi = i;
          if (mismatchedSpectra && table)
          {
            rhs_wi = (*table)[i];
            if (rhs_wi < 0)
              continue;
          }
          else
          {
            // Check for masking except when mismatched sizes
            if(!propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
              continue;
          }
          //Reach here? Do the division
          performBinaryOperation(m_lhs->readX(i),m_lhs->readY(i),m_lhs->readE(i),m_rhs->readY(rhs_wi),m_rhs->readE(rhs_wi),m_out->dataY(i),m_out->dataE(i));

          //Free up memory on the RHS if that is possible
          if (m_ClearRHSWorkspace)
            const_cast<EventList&>(m_erhs->getEventList(rhs_wi)).clear();

          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION
      }
      // Make sure we don't use outdated MRU
      if (m_ClearRHSWorkspace)
        m_erhs->clearMRU();
    }





//
//
//
//
//
//    void BinaryOperation::do2D()
//    {
//      // Propagate any masking first or it could mess up the numbers
//      propagateBinMasks(m_rhs,m_out);
//      // Loop over the spectra calling the virtual function for each one
//      const int numHists = m_lhs->getNumberHistograms();
//      PARALLEL_FOR3(m_lhs,m_rhs,m_out)
//      for (int i = 0; i < numHists; ++i)
//      {
//        PARALLEL_START_INTERUPT_REGION
//        m_out->setX(i,m_lhs->refX(i));
//        if( propagateSpectraMask(m_lhs, m_rhs, i, m_out) )
//        {
//          performBinaryOperation(m_lhs->readX(i),m_lhs->readY(i),m_lhs->readE(i),m_rhs->readY(i),m_rhs->readE(i),m_out->dataY(i),m_out->dataE(i));
//        }
//        m_progress->report();
//        PARALLEL_END_INTERUPT_REGION
//      }
//      PARALLEL_CHECK_INTERUPT_REGION
//    }
//



    //---------------------------------------------------------------------------------------------
    /** Copies any bin masking from the smaller/rhs input workspace to the output.
     *  Masks on the other input workspace are copied automatically by the workspace factory.
     *  @param rhs :: The workspace which is the right hand operand
     *  @param out :: The result workspace
     */
    void BinaryOperation::propagateBinMasks(const API::MatrixWorkspace_const_sptr rhs, API::MatrixWorkspace_sptr out)
    {
      const int outHists = out->getNumberHistograms();
      const int rhsHists = rhs->getNumberHistograms();
      for (int i = 0; i < outHists; ++i)
      {
        // Copy over masks from the rhs, if any exist.
        // If rhs is single spectrum, copy masks from that to all spectra in the output.
        if ( rhs->hasMaskedBins((rhsHists==1) ? 0 : i) )
        {
          const MatrixWorkspace::MaskList & masks = rhs->maskedBins( (rhsHists==1) ? 0 : i );
          MatrixWorkspace::MaskList::const_iterator it;
          for (it = masks.begin(); it != masks.end(); ++it)
          {
            out->maskBin(i,it->first,it->second);
          }
        }
      }
    }


    //---------------------------------------------------------------------------------------------
    /**
     * Apply the requested masking to the output workspace
     * @param out :: The workspace to mask
     */
    void BinaryOperation::applyMaskingToOutput(API::MatrixWorkspace_sptr out)
    {
      int nindices = static_cast<int>(m_indicesToMask.size());
      ParameterMap &pmap = out->instrumentParameters();
      PARALLEL_FOR1(out)
      for(int i = 0; i < nindices; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        try
        {
          IDetector_sptr det_out = out->getDetector(m_indicesToMask[i]);
          PARALLEL_CRITICAL(BinaryOperation_masking)
          {
            pmap.addBool(det_out.get(), "masked", true);
          }
        }
        catch(std::runtime_error &)
        {
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }


    // ------- Default implementations of Event binary operations --------

    /**
     * Carries out the binary operation IN-PLACE on a single EventList,
     * with another EventList as the right-hand operand.
     * The event lists simply get appended.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhs :: Const reference to the EventList on the right hand side.
     */
    void BinaryOperation::performEventBinaryOperation(DataObjects::EventList & lhs,
        const DataObjects::EventList & rhs)
    {
      (void) lhs; (void) rhs; //Avoid compiler warnings
      throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
    }

    /**
     * Carries out the binary operation IN-PLACE on a single EventList,
     * with another (histogrammed) spectrum as the right-hand operand.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsX :: The vector of rhs X bin boundaries
     *  @param rhsY :: The vector of rhs data values
     *  @param rhsE :: The vector of rhs error values
     */
    void BinaryOperation::performEventBinaryOperation(DataObjects::EventList & lhs,
        const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE)
    {
      (void) lhs;  //Avoid compiler warnings
      (void) rhsX; (void) rhsY; (void) rhsE;
      throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
    }

    /**
     * Carries out the binary operation IN-PLACE on a single EventList,
     * with a single (double) value as the right-hand operand
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsY :: The rhs data value
     *  @param rhsE :: The rhs error value
     */
    void BinaryOperation::performEventBinaryOperation(DataObjects::EventList & lhs,
        const double& rhsY, const double& rhsE)
    {
      (void) lhs;  //Avoid compiler warnings
      (void) rhsY; (void) rhsE;
      throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
    }



    //---------------------------------------------------------------------------------------------
    /**
     * Get the type of operand from a workspace
     * @param ws :: workspace to check
     * @return OperandType describing what type of workspace it will be operated as.
     */
    OperandType BinaryOperation::getOperandType(const API::MatrixWorkspace_const_sptr ws)
    {
      //An event workspace?
      EventWorkspace_const_sptr ews = boost::dynamic_pointer_cast<const EventWorkspace>(ws);
      if (ews)
        return eEventList;

      //If the workspace has no axes, then it is a WorkspaceSingleValue
      if (!ws->axes())
        return eNumber;

      //TODO: Check if it is a single-colum one, then
      //  return Number;

      //Otherwise, we take it as a histogram (workspace 2D)
      return eHistogram;
    }


    //---------------------------------------------------------------------------------------------
    /** Check what operation will be needed in order to apply the operation
     * to these two types of workspaces. This function must be overridden
     * and checked against all 9 possible combinations.
     *
     * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
     */
    void BinaryOperation::checkRequirements()
    {

      //In general, the X sizes have to match.
      //  (only for some EventWorkspace cases does it NOT matter...)
      m_matchXSize = true;

      //Operations are not always commutative! Don't flip sides.
      m_flipSides = false;

      //And in general, EventWorkspaces get turned to Workspace2D
      m_keepEventWorkspace = false;

      // This will be set to true for Divide/Multiply
      m_useHistogramForRhsEventWorkspace = false;
    }

    //---------------------------------------------------------------------------------------------
    /** Build up an BinaryOperationTable for performing a binary operation
     * e.g. lhs = (lhs + rhs)
     * where the spectra in rhs are to go into lhs.
     * This function looks to match the detector IDs in rhs to those in the lhs.
     *
     * @param lhs :: matrix workspace in which the operation is being done.
     * @param rhs :: matrix workspace on the right hand side of the operand
     * @return map from detector ID to workspace index for the RHS workspace.
     *        NULL if there is not a 1:1 mapping from detector ID to workspace index (e.g more than one detector per pixel).
     */
    BinaryOperation::BinaryOperationTable * BinaryOperation::buildBinaryOperationTable(MatrixWorkspace_const_sptr lhs, MatrixWorkspace_const_sptr rhs)
    {
      //An addition table is a list of pairs:
      //  First int = workspace index in the EW being added
      //  Second int = workspace index to which it will be added in the OUTPUT EW. -1 if it should add a new entry at the end.
      BinaryOperationTable * table = new BinaryOperationTable();

      // Get the spectra detector maps
      const SpectraDetectorMap & lhs_spec_det_map = lhs->spectraMap();
      const SpectraDetectorMap & rhs_spec_det_map = rhs->spectraMap();

      int rhs_nhist = rhs->getNumberHistograms();
      int lhs_nhist = lhs->getNumberHistograms();

      // Initialize the table; filled with -1 meaning no match
      table->resize(lhs_nhist, -1);

      // We'll need maps from WI to Spectrum Number.
      Timer timer1;
      API::IndexToIndexMap * lhs_wi_to_spec = lhs->getWorkspaceIndexToSpectrumMap();
      API::IndexToIndexMap * rhs_wi_to_spec = rhs->getWorkspaceIndexToSpectrumMap();
      //std::cout << timer1.elapsed() << " sec to getWorkspaceIndexToSpectrumMap\n";

      API::IndexToIndexMap * rhs_det_to_wi;
      rhs_det_to_wi = rhs->getDetectorIDToWorkspaceIndexMap(false);
      //std::cout << timer1.elapsed() << " sec to getDetectorIDToWorkspaceIndexMap\n";

      //PARALLEL_FOR_NO_WSP_CHECK()
      for (int lhsWI = 0; lhsWI < lhs_nhist; lhsWI++)
      {
        int rhs_spec_no;
        bool done=false;

        // Spectrum number for this lhs workspace index.
        int lhs_spec_no = (*lhs_wi_to_spec)[lhsWI];
        // List of detectors on lhs side
        std::vector<int> lhsDets = lhs_spec_det_map.getDetectors(lhs_spec_no);
        // For proper includes, it needs to be sorted
        std::sort(lhsDets.begin(), lhsDets.end());


        // ----------------- Matching Workspace Indices and Detector IDs --------------------------------------
        //First off, try to match the workspace indices. Most times, this will be ok right away.
        int rhsWI = lhsWI;
        if (rhsWI < rhs_nhist) //don't go out of bounds
        {
          // Get the detector IDs at that workspace index.
          rhs_spec_no = (*rhs_wi_to_spec)[rhsWI];
          std::vector<int> rhsDets = rhs_spec_det_map.getDetectors(rhs_spec_no);
          std::sort(rhsDets.begin(), rhsDets.end());

          //Checks that lhsDets is a subset of rhsDets
          if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(), lhsDets.end()))
          {
            //We found the workspace index right away. No need to keep looking
            (*table)[lhsWI] = rhsWI;
            done = true;
          }
        }


        // ----------------- Scrambled Detector IDs with one Detector per Spectrum --------------------------------------
        if (!done && rhs_det_to_wi && (lhsDets.size() == 1))
        {
          //Didn't find it. Try to use the RHS map.

          //First, we have to get the (single) detector ID of the LHS
          std::vector<int>::const_iterator lhsDets_it = lhsDets.begin();
          int lhs_detector_ID = *lhsDets_it;

          //Now we use the RHS map to find it. This only works if both the lhs and rhs have 1 detector per pixel
          IndexToIndexMap::iterator map_it = rhs_det_to_wi->find(lhs_detector_ID);
          if (map_it != rhs_det_to_wi->end())
          {
            rhsWI = map_it->second; //This is the workspace index in the RHS that matched lhs_detector_ID
          }
          else
          {
            //Did not find it!
            rhsWI = -1; //Marker to mean its not in the LHS.

//            std::ostringstream mess;
//            mess << "BinaryOperation: cannot find a RHS spectrum that contains the detectors in LHS workspace index " << lhsWI << "\n";
//            throw std::runtime_error(mess.str());
          }
          (*table)[lhsWI] = rhsWI;
          done = true; //Great, we did it.
        }


        // ----------------- LHS detectors are subset of RHS, which are Grouped --------------------------------------
        if (!done)
        {

          //Didn't find it? Now we need to iterate through the output workspace to
          //  match the detector ID.
          // NOTE: This can be SUPER SLOW!
          for (rhsWI=0; rhsWI < rhs_nhist; rhsWI++)
          {
            rhs_spec_no = (*rhs_wi_to_spec)[rhsWI];
            std::vector<int> rhsDets = rhs_spec_det_map.getDetectors(rhs_spec_no);
            std::sort(rhsDets.begin(), rhsDets.end());

            //Checks that lhsDets is a subset of rhsDets
            if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(), lhsDets.end()))
            {
              //This one is right. Now we can stop looking.
              (*table)[lhsWI] = rhsWI;
              done = true;
              continue;
            }
          }
        }

        // ------- Still nothing ! -----------
        if (!done)
        {
          (*table)[lhsWI] = -1;

//          std::ostringstream mess;
//          mess << "BinaryOperation: cannot find a RHS spectrum that contains the detectors in LHS workspace index " << lhsWI << "\n";
//          throw std::runtime_error(mess.str());
        }
      }
      //std::cout << timer1.elapsed() << " sec to do the rest\n";

      return table;
    }






  } // namespace Algorithms
} // namespace Mantid
