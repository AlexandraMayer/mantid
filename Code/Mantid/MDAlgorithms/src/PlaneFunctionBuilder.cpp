﻿#include "MantidMDAlgorithms/PlaneFunctionBuilder.h"
#include "MantidMDAlgorithms/InvalidParameter.h"
#include "MantidMDAlgorithms/OriginParameter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include <exception>

namespace Mantid
{
    namespace MDAlgorithms
    {
        using namespace Mantid::API;

        PlaneFunctionBuilder::PlaneFunctionBuilder()
        {
        }

        void  PlaneFunctionBuilder::addNormalParameter(NormalParameter& parameter)
        {
            
            this->m_normal = NormalParameter(parameter);
        }

        void  PlaneFunctionBuilder::addOriginParameter(OriginParameter& parameter)
        { 
           
            this->m_origin = OriginParameter(parameter);
        }

        std::auto_ptr<ImplicitFunction> PlaneFunctionBuilder::create() const
        {
            //check that builder parameters are valid.
            if(!m_origin.isValid())
            {
                std::string message = "Invalid origin parameter on PlaneFunctionBuilder";
                throw std::invalid_argument(message);
            } 
            if(!m_normal.isValid())
            {
                std::string message = "Invalid normal parameter passed to PlaneFunctionBuilder";
                throw std::invalid_argument(message);
            }
            //implement construction.
            NormalParameter& refNormal = m_normal;
            OriginParameter& refOrigin = m_origin;
            PlaneImplicitFunction* func = new Mantid::MDAlgorithms::PlaneImplicitFunction(refNormal, refOrigin);
            return std::auto_ptr<ImplicitFunction>(func);
        }

        PlaneFunctionBuilder::~PlaneFunctionBuilder()
        {
        }
    }


}
