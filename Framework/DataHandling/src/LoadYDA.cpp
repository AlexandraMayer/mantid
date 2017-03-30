#include "MantidDataHandling/LoadYDA.h"

#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/WorkspaceFactory.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/UnitFactory.h"

#include "yaml-cpp/yaml.h"
#include <iostream>
#include <math.h>




namespace Mantid {
namespace DataHandling {
API::MatrixWorkspace_sptr errorws();
using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

size_t xLength;
size_t yLength;
size_t histLength;
YAML::Node f;

void addSampleLogData( API::MatrixWorkspace_sptr ws,const std::string &name,const std::string &value ) {
    API::Run &run = ws->mutableRun();
    run.addLogData(new Mantid::Kernel::PropertyWithValue<std::string>(name,value));
}

void addSampleLogData(API::MatrixWorkspace_sptr ws, const std::string &name,const double &value) {
    API::Run &run = ws->mutableRun();
    run.addLogData(new Mantid::Kernel::PropertyWithValue<double>(name,value));
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadYDA)

//----------------------------------------------------------------------------------------------

LoadYDA::LoadYDA() {}

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadYDA::category() const {
  return "DataHandling\\Text";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadYDA::summary() const {
  return "Loads data from a yaml file and stores it into a 2D workspace";
}

int LoadYDA::confidence(Kernel::FileDescriptor &descriptor) const {
    if((descriptor.extension() != ".yaml")||(descriptor.extension() != ".txt")) {
        return 0;
    }
    YAML::Node fileN = YAML::LoadFile(descriptor.filename());
    if(fileN.IsNull()) {
        return 0;
    }
    auto &file = descriptor.data();

    std::string fileline;
    int confidence(0);

    std::getline(file,fileline);
    if(fileline.find("Meta:") != std::string::npos) {
        confidence += 20;
        std::getline(file,fileline);
        if(fileline.find("format:") != std::string::npos && fileline.find("yaml") != std::string::npos) {
            confidence += 20;
            std::getline(file,fileline);
            if(fileline.find("type: generic tabular data") != std::string::npos) {
                confidence += 20;
            }
        }
    } else {
        confidence = 0;
    }
    g_log.debug("confidence");
    g_log.debug(std::to_string(confidence));
    return confidence;

}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadYDA::init() {
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "",API::FileProperty::Load,""),
      "The name of the file to read.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", "",Kernel::Direction::Output),
      "The name to use for the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadYDA::exec() {

  const std::string filename = getPropertyValue("Filename");
  try {
    f =  YAML::LoadFile(filename);
  } catch (const std::exception e) {
  g_log.debug("abfrage so moglich");
  f = YAML::Null;
  }

  //YAML::LoadFile(filename);
  g_log.debug("no problem with initialzation");

  g_log.debug(std::to_string(f.IsNull()));
  for(YAML::const_iterator it = f.begin(); it != f.end();++it) {
      g_log.debug(it->first.as<std::string>());
  }
  if(f.IsNull()) {
      g_log.warning("Error reading file " + filename);
  }


  API::MatrixWorkspace_sptr ws = nullptr;

  g_log.debug("Problem with slices?");
  YAML::Node slices = f["Slices"];


  if(slices.IsNull() || slices.size() <= 0) {
      g_log.warning("No slices data found");
      histLength = 0;
  } else {
  xLength = slices[0]["x"].size() + 1;
  g_log.debug(std::to_string(xLength));
  yLength = slices[0]["y"].size();
    histLength = slices.size();
  g_log.debug(std::to_string(histLength));

  }
  if(histLength <= 0) {
      g_log.warning("slices to small!");
      API::MatrixWorkspace_sptr error = errorws();
      setProperty("OutputWorkspace",error);
  } else {

      ws = setupWs();




      g_log.debug(f["History"][0].as<std::string>());

      YAML::Node hist = f["History"];
      if(hist.IsNull() || hist.size() < 4) {
        g_log.warning("History must have 4 or more arguments");
      } else {
          //g_log.debug(std::to_string(hist.size()));
          std::string propn = hist[0].as<std::string>();
          propn = propn.back();
          //g_log.debug(propn);
          addSampleLogData(ws,"poposal_number",propn);
          std::string propt = hist[1].as<std::string>();
          //g_log.debug(propt);
          addSampleLogData(ws,"poposal_title",propt);
          std::string expteam = hist[2].as<std::string>();
          //g_log.debug(expteam);
          addSampleLogData(ws,"experiment_team",expteam);
      }

      YAML::Node rpar = f["RPar"];
      if(rpar.IsNull() || rpar.size() < 2) {
        g_log.warning("Rpar has less than 2 arguments or was not found");
      } else {
          std::string temp = rpar[0]["val"].as<std::string>();
          //g_log.debug(temp);
          addSampleLogData(ws,"temperature", temp);
          std::string ei = rpar[1]["val"].as<std::string>();
          //g_log.debug(ei);
          addSampleLogData(ws,"Ei",ei);
      }
      YAML::Node coord = f["Coord"];

      if(coord.IsNull() || coord.size() < 3) {
        g_log.warning("Coord has less than 3 arguments or was not found");
      } else {
          std::string z = coord["z"]["name"].as<std::string>();
          if(coord["z"].IsNull()) {
              g_log.warning("No z coordinate found");
          } else if(coord["z"]["name"].IsNull()) {
              g_log.warning("No name for z found");
          } else {
          //g_log.debug(z);
              if(z == "q") {
                  ws->getAxis(1)->unit() = Kernel::UnitFactory::Instance().create("MomentumTransfer");
              } else {
                  ws->setYUnitLabel("Counts");
              }
          }
      }


      std::vector<std::vector<double>> xAxis;
      std::vector<std::vector<double>> yAxis;
      std::vector<std::vector<double>> eAxis;

      getAxisVal(xAxis,yAxis,eAxis);

      //g_log.debug(slices[0]["x"][0].as<std::string>());
    /*
      for(unsigned int i = 0; i < slices.size(); i++) {
          auto xati = slices[i]["x"];
          g_log.debug(xati[0].as<std::string>());
          double diff = xati[1].as<double>() - xati[0].as<double>();
          g_log.debug(std::to_string(diff));
          double first = round( (((xati[0].as<double>()-diff)+xati[0].as<double>())/2) * 10000.0 ) / 10000.0;
          g_log.debug(std::to_string(first));

      }
    */
      for(size_t i = 0; i < histLength; i++) {
          ws->mutableX(i) = xAxis.at(i);
          ws->mutableY(i) = yAxis.at(i);
          ws->mutableE(i) = eAxis.at(i);
      }
    setProperty("OutputWorkspace",ws);
   }


}

API::MatrixWorkspace_sptr LoadYDA::setupWs() const {
    API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
                API::WorkspaceFactory::Instance().create("Workspace2D",histLength,xLength,yLength));
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("DeltaE");

    return ws;
}

API::MatrixWorkspace_sptr errorws() {
    API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(API::WorkspaceFactory::Instance().create("Workspace2D",1,1,1));
    return ws;
}

void LoadYDA::getAxisVal(std::vector<std::vector<double> > &x, std::vector<std::vector<double> > &y, std::vector<std::vector<double> > &e) {
    YAML::Node slices = f["Slices"];
    if(slices.size() <= 0) {
        g_log.warning("No slices in this file");
    } else {
        for(unsigned int i = 0; i < slices.size(); i++) {
            YAML::Node xati = slices[i]["x"];

            std::vector<double> xs;
            if(xati.size() <= 0) {
                g_log.warning("Not enough x values in slices");
            } else if(xati.size() == 1) {
                xs.push_back(xati[0].as<double>());
                x.push_back(xs);
            } else {
                double diff = xati[1].as<double>() - xati[0].as<double>();
                //g_log.debug(std::to_string(diff));
                double first = round( (((xati[0].as<double>()-diff)+xati[0].as<double>())/2) * 10000.0 ) / 10000.0;
                //g_log.debug(std::to_string(first));

                xs.push_back(first);
                for(unsigned int j = 0; j < xati.size();j++) {
                    xs.push_back(first+diff);
                    first = first + diff;
                    //g_log.debug(std::to_string(first));
                }
                x.push_back(xs);
            }

            YAML::Node yati = slices[i]["y"];

            if(yati.size() <= 0) {
                g_log.warning("Not enough y values in slices");
            } else {
                std::vector<double> ys;
                std::vector<double> es;


                for(unsigned int j = 0; j < yati.size();j++) {
                    ys.push_back(yati[j].as<double>());
                   // g_log.debug(yati[j].as<std::string>());
                    es.push_back(sqrt(yati[j].as<double>()));
                }
                y.push_back(ys);
                e.push_back(es);
            }

        }
    }
}

} // namespace DataHandling
} // namespace Mantid
