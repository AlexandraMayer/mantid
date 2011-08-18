#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include "MantidMDAlgorithms/CompositeImplicitFunctionParser.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        DECLARE_IMPLICIT_FUNCTION_PARSER(CompositeImplicitFunctionParser);

        CompositeImplicitFunctionParser::CompositeImplicitFunctionParser() : ImplicitFunctionParser(new InvalidParameterParser)
        {
        }

        API::ImplicitFunctionBuilder* CompositeImplicitFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
        {
            Mantid::API::ImplicitFunctionBuilder* functionBuilder;
            if("Function" != functionElement->localName())
            {
               std::string message = "This is not a function element: " + functionElement->localName(); 
               throw std::invalid_argument(message);
            }
            
            std::string type = functionElement->getChildElement("Type")->innerText();
            if(CompositeImplicitFunction::functionName() != type)
            {
                ImplicitFunctionParser::checkSuccessorExists();
                functionBuilder = m_successor->createFunctionBuilder(functionElement);
            }
            else
            {
                functionBuilder = parseCompositeFunction(functionElement);
            }
            return functionBuilder;
        }

        void CompositeImplicitFunctionParser::setSuccessorParser(ImplicitFunctionParser* parser)
        {
          ImplicitFunctionParser::SuccessorType temp(parser);
          this->m_successor.swap(temp);
        }

        CompositeFunctionBuilder * CompositeImplicitFunctionParser::parseCompositeFunction(Poco::XML::Element* functionElement)
        {
            using namespace Poco::XML;
            //ImplicitFunctionParser::checkSuccessorExists();
            boost::interprocess::unique_ptr<CompositeFunctionBuilder, Mantid::API::DeleterPolicy<CompositeFunctionBuilder> > functionBuilder(new CompositeFunctionBuilder);
            NodeList* childFunctionElementList = functionElement->childNodes();
           
            for(unsigned long i = 0; i < childFunctionElementList->length(); i++)
            {
                Element* childFunctionElement = dynamic_cast<Element*>(childFunctionElementList->item(i));
                std::string typeName = childFunctionElement->localName();
                if("Function" == typeName)
                {
                    Mantid::API::ImplicitFunctionBuilder* childFunctionBuilder = this->createFunctionBuilder(childFunctionElement);

                    functionBuilder->addFunctionBuilder(childFunctionBuilder);
                }
            }
            
            return functionBuilder.release(); 
        }

        CompositeImplicitFunctionParser::~CompositeImplicitFunctionParser()
        {
        }

        void CompositeImplicitFunctionParser::setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
        {
          Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(parser);
          this->m_paramParserRoot.swap(temp);
        }
    }
}
