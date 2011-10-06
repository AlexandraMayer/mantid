//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Run.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/lexical_cast.hpp>
#include "MantidAPI/PropertyNexus.h"

namespace Mantid
{
namespace API
{

using namespace Kernel;

const int Run::ADDABLES = 6;
const std::string Run::ADDABLE[ADDABLES] = {"tot_prtn_chrg", "rawfrm", "goodfrm", "dur", "gd_prtn_chrg", "uA.hour"};

// Get a reference to the logger
Kernel::Logger& Run::g_log = Kernel::Logger::get("Run");

  //----------------------------------------------------------------------
  // Public member functions
  //----------------------------------------------------------------------
  /**
   * Default constructor
   */
  Run::Run() : m_manager(), m_protonChargeName("gd_prtn_chrg"), 
    m_goniometer()
  {
  }

  /**
   * Destructor
   */
  Run::~Run()
  {
  }

  /**
   * Copy constructor
   * @param copy :: The object to initialize the copy from
   */
  Run::Run(const Run& copy) : m_manager(copy.m_manager), m_protonChargeName(copy.m_protonChargeName), 
    m_goniometer(copy.m_goniometer)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Assignment operator
   * @param rhs :: The object whose properties should be copied into this
   * @returns A cont reference to the copied object
   */
  const Run& Run::operator=(const Run& rhs)
  {
    if( this == &rhs ) return *this;
    m_manager = rhs.m_manager;
    m_protonChargeName = rhs.m_protonChargeName;
    m_goniometer = rhs.m_goniometer;
    return *this;
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Adds just the properties that are safe to add. All time series are
   * merged together and the list of addable properties are added
   * @param rhs The object that is being added to this.
   * @returns A reference to the summed object
   */
  Run& Run::operator+=(const Run& rhs)
  {
    //merge and copy properties where there is no risk of corrupting data
    mergeMergables(m_manager, rhs.m_manager);

    // Other properties are added to gether if they are on the approved list
    for(int i = 0; i < ADDABLES; ++i )
    {
      // get a pointer to the property on the right-handside workspace
      Property * right;
      try
      {
        right = rhs.m_manager.getProperty(ADDABLE[i]);
      }
      catch (Exception::NotFoundError &)
      {
        //if it's not there then ignore it and move on
        continue;
      }
      // now deal with the left-handside
      Property * left;
      try
      {
        left = m_manager.getProperty(ADDABLE[i]);
      }
      catch (Exception::NotFoundError &)
      {
        //no property on the left-handside, create one and copy the right-handside across verbatum
        m_manager.declareProperty(right->clone(), "");
        continue;
      }
      
      left->operator+=(right);

    }
    return *this;
  }

  /**
  * Set the run start and end
  * @param start :: The run start
  * @param end :: The run end
  */
  void Run::setStartAndEndTime(const Kernel::DateAndTime & start, const Kernel::DateAndTime & end)
  {
    this->addProperty<std::string>("start_time", start.to_ISO8601_string(), true);
    this->addProperty<std::string>("end_time", end.to_ISO8601_string(), true);
  }

  /// Return the run start time
  const Kernel::DateAndTime Run::startTime() const
  {
    const std::string start_prop("start_time");
    if( this->hasProperty(start_prop) ) 
    {
      std::string start = this->getProperty(start_prop)->value();
      return DateAndTime(start);
    }
    else
    {
      throw std::runtime_error("Run::startTime() - No start time has been set for this run.");
    }
  }

  /// Return the run end time
  const Kernel::DateAndTime Run::endTime() const
  {
    const std::string end_prop("end_time");
    if( this->hasProperty(end_prop) )
    {
      std::string end = this->getProperty(end_prop)->value();
      return DateAndTime(end);
    }
    else
    {
      throw std::runtime_error("Run::endTime() - No end time has been set for this run.");
    }
  }

  /** Adds all the time series in the second property manager to those in the first
  * @param sum the properties to add to
  * @param toAdd the properties to add
  */
  void Run::mergeMergables(Mantid::Kernel::PropertyManager & sum, const Mantid::Kernel::PropertyManager & toAdd)
  {
    // get pointers to all the properties on the right-handside and prepare to loop through them
    const std::vector<Property*> inc = toAdd.getProperties();
    std::vector<Property*>::const_iterator end = inc.end();
    for (std::vector<Property*>::const_iterator it=inc.begin(); it != end;++it)
    {
      const std::string rhs_name = (*it)->name();
      try
      {
        //now get pointers to the same properties on the left-handside
        Property * lhs_prop(sum.getProperty(rhs_name));
        lhs_prop->merge(*it);
        
/*        TimeSeriesProperty * timeS = dynamic_cast< TimeSeriesProperty * >(lhs_prop);
        if (timeS)
        {
          (*lhs_prop) += (*it);
        }*/
      }
      catch (Exception::NotFoundError &)
      {
        //copy any properties that aren't already on the left hand side
        Property * copy = (*it)->clone();
        //And we add a copy of that property to *this
        sum.declareProperty(copy, "");
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  /**
   * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
   *  absolute time range.
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param start :: Absolute start time. Any log entries at times >= to this time are kept.
   * @param stop :: Absolute stop time. Any log entries at times < than this time are kept.
   */
  void Run::filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop)
  {
    //The propery manager operator will make all timeseriesproperties filter.
    m_manager.filterByTime(start, stop);

    //Re-integrate proton charge
    this->integrateProtonCharge(start, stop);
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Split a run by time (splits the TimeSeriesProperties contained).
   *
   * Total proton charge will get re-integrated after filtering.
   *
   * @param splitter :: TimeSplitterType with the intervals and destinations.
   * @param outputs :: Vector of output runs.
   */
  void Run::splitByTime(TimeSplitterType& splitter, std::vector< Run * > outputs) const
  {
    //Make a vector of managers for the splitter. Fun!
    std::vector< PropertyManager *> output_managers;
    size_t n = outputs.size();
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
        output_managers.push_back( &(outputs[i]->m_manager) );
      else
        output_managers.push_back( NULL );
    }

    //Now that will do the split down here.
    m_manager.splitByTime(splitter, output_managers);

    //Re-integrate proton charge of all outputs
    for (size_t i=0; i<n; i++)
    {
      if (outputs[i])
        outputs[i]->integrateProtonCharge();
    }
  }



  //-----------------------------------------------------------------------------------------------
  /**
   * Add data to the object in the form of a property
   * @param prop :: A pointer to a property whose ownership is transferred to this object
   * @param overwrite :: If true, a current value is overwritten. (Default: False)
   */
  void Run::addProperty(Kernel::Property *prop, bool overwrite)
  {
    // Mmake an exception for the proton charge
    // and overwrite it's value as we don't want to store the proton charge in two separate locations
    // Similar we don't want more than one run_title
    std::string name = prop->name();
    if( hasProperty(name) && (overwrite || prop->name() == m_protonChargeName || prop->name()=="run_title") )
    {
      removeProperty(name);
    }
    m_manager.declareProperty(prop, "");
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Set the good proton charge total for this run
   *  @param charge :: The proton charge in uA.hour
   */
  void Run::setProtonCharge(const double charge)
  {
    if( !hasProperty(m_protonChargeName) )
    {
      addProperty(m_protonChargeName, charge, "uA.hour");
    }
    else
    {
      Kernel::Property *charge_prop = getProperty(m_protonChargeName);
      charge_prop->setValue(boost::lexical_cast<std::string>(charge));
    }
  }


  //-----------------------------------------------------------------------------------------------
  /**
   * Retrieve the total good proton charge delivered in this run
   * @return The proton charge in uA.hour
   * @throw Exception::NotFoundError if the proton charge has not been set
   */
  double Run::getProtonCharge() const
  {
    double charge = m_manager.getProperty(m_protonChargeName);
    return charge;
  }

  //-----------------------------------------------------------------------------------------------
  /// Integrate the proton charge over a whole run range
  double Run::integrateProtonCharge()
  {
    return integrateProtonCharge(startTime(), endTime());
  }

  /**
   * Calculate the total proton charge by integrating up all the entries in the
   * "proton_charge" time series log. This is then saved in the log entry
   * using setProtonCharge().
   * If "proton_charge" is not found, the value is set to 0.0.
   * @param start The start of the integration time
   * @param end The end of the integration time
   * @return :: the total charge in microAmp*hours.
   */
  double Run::integrateProtonCharge(const Kernel::DateAndTime & start, const Kernel::DateAndTime & end)
  {
    Kernel::TimeSeriesProperty<double> * log;

    try
    {
      log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( this->getProperty("proton_charge") );
    }
    catch (Exception::NotFoundError &)
    {
      //g_log.information() << "proton_charge log value not found. Total proton charge set to 0.0\n";
      this->setProtonCharge(0);
      return 0;
    }

    if (log)
    {
      double total = log->getTotalValue();
      std::string unit = log->units();
      // Do we need to take account of a unit
      if(unit.find("picoCoulomb") != std::string::npos )
      {
        /// Conversion factor between picoColumbs and microAmp*hours
        const double currentConversion = 1.e-6 / 3600.;
        total *= currentConversion;
      }
      else if(!unit.empty() && unit != "uAh")
      {
        g_log.warning("Proton charge log has units other than uAh or picoCoulombs. The value of the total proton charge has been left at the sum of the log values.");
      }
      this->setProtonCharge(total);
      return total;
    }
    else
    {
      return -1;
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Return the total memory used by the run object, in bytes.
   */
  size_t Run::getMemorySize() const
  {
    size_t total = 0;
    std::vector< Property*> props = m_manager.getProperties();
    for (size_t i=0; i < props.size(); i++)
    {
      Property * p = props[i];
      if (p)
        total += p->getMemorySize() + sizeof(Property *);
    }
    return total;
  }



  //-----------------------------------------------------------------------------------------------
  /** Get the gonimeter rotation matrix, calculated using the
   * previously set Goniometer object as well as the angles
   * loaded in the run (if any).
   *
   * As of now, it uses the FIRST angle value found.
   *
   * @return 3x3 double rotation matrix
   */
  Mantid::Kernel::DblMatrix Run::getGoniometerMatrix()
  {
    for (size_t i=0; i < m_goniometer.getNumberAxes(); ++i)
    {
      std::string name = m_goniometer.getAxis(i).name;
      if (this->hasProperty(name))
      {
        Property * prop = this->getProperty(name);
        TimeSeriesProperty<double> * tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
        if (tsp)
        {
          // Set that angle
          m_goniometer.setRotationAngle(i, tsp->firstValue());
          g_log.debug() << "Goniometer angle " << name << " set to " << tsp->firstValue() << std::endl;
        }
        else
          throw std::runtime_error("Sample log for goniometer angle '" + name + "' was not a TimeSeriesProperty<double>.");
      }
      else
        throw std::runtime_error("Could not find goniometer angle '" + name + "' in the run sample logs.");
    }
    return m_goniometer.getR();
  }




  //--------------------------------------------------------------------------------------------
  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to create
   */
  void Run::saveNexus(::NeXus::File * file, const std::string & group) const
  {
    file->makeGroup(group, "NXgroup", 1);
    file->putAttr("version", 1);

    // Now the goniometer
    m_goniometer.saveNexus(file, "goniometer");

    // Save all the properties as NXlog
    std::vector<Property *> props = m_manager.getProperties();
    for (size_t i=0; i<props.size(); i++)
    {
      Property * prop = props[i];
      if (prop)
        PropertyNexus::saveProperty(file, prop);
    }
    file->closeGroup();
  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to open. Empty string to NOT open a group, but
   * load any NXlog in the current open group.
   */
  void Run::loadNexus(::NeXus::File * file, const std::string & group)
  {
    if (!group.empty()) file->openGroup(group, "NXgroup");

    std::map<std::string, std::string> entries;
    file->getEntries(entries);
    std::map<std::string, std::string>::iterator it = entries.begin();
    std::map<std::string, std::string>::iterator it_end = entries.end();
    for (; it != it_end; ++it)
    {
      // Get the name/class pair
      const std::pair<std::string, std::string> & name_class = *it;
      // NXLog types are the main one.
      if (name_class.second == "NXlog")
      {
        Property * prop = PropertyNexus::loadProperty(file, name_class.first);
        if (prop)
        {
          if (m_manager.existsProperty(prop->name() ))
            m_manager.removeProperty(prop->name() );
          m_manager.declareProperty(prop);
        }
      }
      else if (name_class.second == "NXpositioner")
      {
        // Goniometer class
        m_goniometer.loadNexus(file, name_class.first);
      }
      else if (name_class.first == "proton_charge")
      {
        // Old files may have a proton_charge field, single value (not even NXlog)
        double charge;
        file->readData("proton_charge", charge);
        this->setProtonCharge(charge);
      }
    }
    if (!group.empty()) file->closeGroup();


    if( this->hasProperty("proton_charge") )
    {
      // Old files may have a proton_charge field, single value.
      // Modern files (e.g. SNS) have a proton_charge TimeSeriesProperty.
      PropertyWithValue<double> *charge_log = dynamic_cast<PropertyWithValue<double>*>(this->getProperty("proton_charge"));
      if (charge_log)
      {  this->setProtonCharge(boost::lexical_cast<double>(charge_log->value()));
      }
    }
  }

} //API namespace

}

