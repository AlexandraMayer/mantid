#ifndef EVENT_WS_DATA_SOURCE_H
#define EVENT_WS_DATA_SOURCE_H

#include <cstddef>

#include "MantidQtImageViewer/DataArray.h"
#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtImageViewer/DllOptionIV.h"

#include "MantidAPI/IEventWorkspace.h"

/**
    @class EventWSDataSource 
  
       This class provides a concrete implementation of an ImageDataSource
    that gets it's data from an event workspace.
 
    @author Dennis Mikkelson 
    @date   2012-04-04 
     
    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories
  
    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER EventWSDataSource: public ImageDataSource
{
  public:

    /// Construct a DataSource object around the specifed EventWorkspace
    EventWSDataSource( Mantid::API::IEventWorkspace_sptr ev_ws );

   ~EventWSDataSource();

    /// OVERRIDES: Get the smallest 'x' value covered by the data
    virtual double GetXMin();

    /// OVERRIDES: Get the largest 'x' value covered by the data
    virtual double GetXMax();

    /// OVERRIDES: Get the largest 'y' value covered by the data
    virtual double GetYMax();

    /// OVERRIDES: Get the total number of rows of data
    virtual size_t GetNRows();

    /// Get DataArray covering full range of data in x, and y directions
    DataArray * GetDataArray( bool is_log_x );

    /// Get DataArray covering restricted range of data 
    DataArray * GetDataArray( double  xmin,
                              double  xmax,
                              double  ymin,
                              double  ymax,
                              size_t  n_rows,
                              size_t  n_cols,
                              bool    is_log_x );

    /// Get a list containing pairs of strings with information about x,y
    void GetInfoList( double x,
                      double y,
                      std::vector<std::string> &list );
  private:
    Mantid::API::IEventWorkspace_sptr  ev_ws;

};

} // namespace MantidQt 
} // namespace ImageView 

#endif // EVENT_WS_DATA_SOURCE_H
