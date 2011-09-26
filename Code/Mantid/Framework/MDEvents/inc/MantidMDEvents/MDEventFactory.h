#ifndef MANTID_MDEVENTS_MDEVENTFACTORY_H_
#define MANTID_MDEVENTS_MDEVENTFACTORY_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include <boost/shared_ptr.hpp>


namespace Mantid
{
namespace MDEvents
{

  /** MDEventFactory : collection of methods
   * to create MDLeanEvent* instances, by specifying the number
   * of dimensions as a parameter.
   *
   * @author Janik Zikovsky
   * @date 2011-02-24 15:08:43.105134
   */
  class DLLExport MDEventFactory
  {
  public:
    MDEventFactory() {}
    ~MDEventFactory() {}
    static API::IMDEventWorkspace_sptr CreateMDWorkspace(size_t nd, const std::string & eventType="MDLeanEvent");
  };

  //### BEGIN AUTO-GENERATED CODE #################################################################
  
  /** Macro that makes it possible to call a templated method for
   * a MDEventWorkspace using a IMDEventWorkspace_sptr as the input.
   *
   * @param funcname :: name of the function that will be called.
   * @param workspace :: IMDEventWorkspace_sptr input workspace.
   */
   
  #define CALL_MDEVENT_FUNCTION(funcname, workspace) \
  { \
  MDEventWorkspace<MDEvent<1>, 1>::sptr MDEW_MDEVENT_1 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<1>, 1> >(workspace); \
  if (MDEW_MDEVENT_1) funcname<MDEvent<1>, 1>(MDEW_MDEVENT_1); \
  MDEventWorkspace<MDEvent<2>, 2>::sptr MDEW_MDEVENT_2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<2>, 2> >(workspace); \
  if (MDEW_MDEVENT_2) funcname<MDEvent<2>, 2>(MDEW_MDEVENT_2); \
  MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW_MDEVENT_3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(workspace); \
  if (MDEW_MDEVENT_3) funcname<MDEvent<3>, 3>(MDEW_MDEVENT_3); \
  MDEventWorkspace<MDEvent<4>, 4>::sptr MDEW_MDEVENT_4 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<4>, 4> >(workspace); \
  if (MDEW_MDEVENT_4) funcname<MDEvent<4>, 4>(MDEW_MDEVENT_4); \
  MDEventWorkspace<MDEvent<5>, 5>::sptr MDEW_MDEVENT_5 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<5>, 5> >(workspace); \
  if (MDEW_MDEVENT_5) funcname<MDEvent<5>, 5>(MDEW_MDEVENT_5); \
  MDEventWorkspace<MDEvent<6>, 6>::sptr MDEW_MDEVENT_6 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<6>, 6> >(workspace); \
  if (MDEW_MDEVENT_6) funcname<MDEvent<6>, 6>(MDEW_MDEVENT_6); \
  MDEventWorkspace<MDEvent<7>, 7>::sptr MDEW_MDEVENT_7 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<7>, 7> >(workspace); \
  if (MDEW_MDEVENT_7) funcname<MDEvent<7>, 7>(MDEW_MDEVENT_7); \
  MDEventWorkspace<MDEvent<8>, 8>::sptr MDEW_MDEVENT_8 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<8>, 8> >(workspace); \
  if (MDEW_MDEVENT_8) funcname<MDEvent<8>, 8>(MDEW_MDEVENT_8); \
  MDEventWorkspace<MDEvent<9>, 9>::sptr MDEW_MDEVENT_9 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<9>, 9> >(workspace); \
  if (MDEW_MDEVENT_9) funcname<MDEvent<9>, 9>(MDEW_MDEVENT_9); \
  MDEventWorkspace<MDLeanEvent<1>, 1>::sptr MDEW_MDLEANEVENT_1 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<1>, 1> >(workspace); \
  if (MDEW_MDLEANEVENT_1) funcname<MDLeanEvent<1>, 1>(MDEW_MDLEANEVENT_1); \
  MDEventWorkspace<MDLeanEvent<2>, 2>::sptr MDEW_MDLEANEVENT_2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<2>, 2> >(workspace); \
  if (MDEW_MDLEANEVENT_2) funcname<MDLeanEvent<2>, 2>(MDEW_MDLEANEVENT_2); \
  MDEventWorkspace<MDLeanEvent<3>, 3>::sptr MDEW_MDLEANEVENT_3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>, 3> >(workspace); \
  if (MDEW_MDLEANEVENT_3) funcname<MDLeanEvent<3>, 3>(MDEW_MDLEANEVENT_3); \
  MDEventWorkspace<MDLeanEvent<4>, 4>::sptr MDEW_MDLEANEVENT_4 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<4>, 4> >(workspace); \
  if (MDEW_MDLEANEVENT_4) funcname<MDLeanEvent<4>, 4>(MDEW_MDLEANEVENT_4); \
  MDEventWorkspace<MDLeanEvent<5>, 5>::sptr MDEW_MDLEANEVENT_5 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<5>, 5> >(workspace); \
  if (MDEW_MDLEANEVENT_5) funcname<MDLeanEvent<5>, 5>(MDEW_MDLEANEVENT_5); \
  MDEventWorkspace<MDLeanEvent<6>, 6>::sptr MDEW_MDLEANEVENT_6 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<6>, 6> >(workspace); \
  if (MDEW_MDLEANEVENT_6) funcname<MDLeanEvent<6>, 6>(MDEW_MDLEANEVENT_6); \
  MDEventWorkspace<MDLeanEvent<7>, 7>::sptr MDEW_MDLEANEVENT_7 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<7>, 7> >(workspace); \
  if (MDEW_MDLEANEVENT_7) funcname<MDLeanEvent<7>, 7>(MDEW_MDLEANEVENT_7); \
  MDEventWorkspace<MDLeanEvent<8>, 8>::sptr MDEW_MDLEANEVENT_8 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<8>, 8> >(workspace); \
  if (MDEW_MDLEANEVENT_8) funcname<MDLeanEvent<8>, 8>(MDEW_MDLEANEVENT_8); \
  MDEventWorkspace<MDLeanEvent<9>, 9>::sptr MDEW_MDLEANEVENT_9 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<9>, 9> >(workspace); \
  if (MDEW_MDLEANEVENT_9) funcname<MDLeanEvent<9>, 9>(MDEW_MDLEANEVENT_9); \
  } 
  
  
  /** Macro that makes it possible to call a templated method for
   * a MDEventWorkspace using a IMDEventWorkspace_sptr as the input.
   *
   * @param funcname :: name of the function that will be called.
   * @param workspace :: IMDEventWorkspace_sptr input workspace.
   */
   
  #define CALL_MDEVENT_FUNCTION3(funcname, workspace) \
  { \
  MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW_MDEVENT_3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(workspace); \
  if (MDEW_MDEVENT_3) funcname<MDEvent<3>, 3>(MDEW_MDEVENT_3); \
  MDEventWorkspace<MDEvent<4>, 4>::sptr MDEW_MDEVENT_4 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<4>, 4> >(workspace); \
  if (MDEW_MDEVENT_4) funcname<MDEvent<4>, 4>(MDEW_MDEVENT_4); \
  MDEventWorkspace<MDEvent<5>, 5>::sptr MDEW_MDEVENT_5 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<5>, 5> >(workspace); \
  if (MDEW_MDEVENT_5) funcname<MDEvent<5>, 5>(MDEW_MDEVENT_5); \
  MDEventWorkspace<MDEvent<6>, 6>::sptr MDEW_MDEVENT_6 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<6>, 6> >(workspace); \
  if (MDEW_MDEVENT_6) funcname<MDEvent<6>, 6>(MDEW_MDEVENT_6); \
  MDEventWorkspace<MDEvent<7>, 7>::sptr MDEW_MDEVENT_7 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<7>, 7> >(workspace); \
  if (MDEW_MDEVENT_7) funcname<MDEvent<7>, 7>(MDEW_MDEVENT_7); \
  MDEventWorkspace<MDEvent<8>, 8>::sptr MDEW_MDEVENT_8 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<8>, 8> >(workspace); \
  if (MDEW_MDEVENT_8) funcname<MDEvent<8>, 8>(MDEW_MDEVENT_8); \
  MDEventWorkspace<MDEvent<9>, 9>::sptr MDEW_MDEVENT_9 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<9>, 9> >(workspace); \
  if (MDEW_MDEVENT_9) funcname<MDEvent<9>, 9>(MDEW_MDEVENT_9); \
  MDEventWorkspace<MDLeanEvent<3>, 3>::sptr MDEW_MDLEANEVENT_3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>, 3> >(workspace); \
  if (MDEW_MDLEANEVENT_3) funcname<MDLeanEvent<3>, 3>(MDEW_MDLEANEVENT_3); \
  MDEventWorkspace<MDLeanEvent<4>, 4>::sptr MDEW_MDLEANEVENT_4 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<4>, 4> >(workspace); \
  if (MDEW_MDLEANEVENT_4) funcname<MDLeanEvent<4>, 4>(MDEW_MDLEANEVENT_4); \
  MDEventWorkspace<MDLeanEvent<5>, 5>::sptr MDEW_MDLEANEVENT_5 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<5>, 5> >(workspace); \
  if (MDEW_MDLEANEVENT_5) funcname<MDLeanEvent<5>, 5>(MDEW_MDLEANEVENT_5); \
  MDEventWorkspace<MDLeanEvent<6>, 6>::sptr MDEW_MDLEANEVENT_6 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<6>, 6> >(workspace); \
  if (MDEW_MDLEANEVENT_6) funcname<MDLeanEvent<6>, 6>(MDEW_MDLEANEVENT_6); \
  MDEventWorkspace<MDLeanEvent<7>, 7>::sptr MDEW_MDLEANEVENT_7 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<7>, 7> >(workspace); \
  if (MDEW_MDLEANEVENT_7) funcname<MDLeanEvent<7>, 7>(MDEW_MDLEANEVENT_7); \
  MDEventWorkspace<MDLeanEvent<8>, 8>::sptr MDEW_MDLEANEVENT_8 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<8>, 8> >(workspace); \
  if (MDEW_MDLEANEVENT_8) funcname<MDLeanEvent<8>, 8>(MDEW_MDLEANEVENT_8); \
  MDEventWorkspace<MDLeanEvent<9>, 9>::sptr MDEW_MDLEANEVENT_9 = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<9>, 9> >(workspace); \
  if (MDEW_MDLEANEVENT_9) funcname<MDLeanEvent<9>, 9>(MDEW_MDLEANEVENT_9); \
  } 
  
  
  /** Macro that makes it possible to call a templated method for
   * a MDEventWorkspace using a IMDEventWorkspace_sptr as the input.
   *
   * @param funcname :: name of the function that will be called.
   * @param workspace :: IMDEventWorkspace_sptr input workspace.
   */
   
  #define CONST_CALL_MDEVENT_FUNCTION(funcname, workspace) \
  { \
  const MDEventWorkspace<MDEvent<1>, 1>::sptr CONST_MDEW_MDEVENT_1 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<1>, 1> >(workspace); \
  if (CONST_MDEW_MDEVENT_1) funcname<MDEvent<1>, 1>(CONST_MDEW_MDEVENT_1); \
  const MDEventWorkspace<MDEvent<2>, 2>::sptr CONST_MDEW_MDEVENT_2 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<2>, 2> >(workspace); \
  if (CONST_MDEW_MDEVENT_2) funcname<MDEvent<2>, 2>(CONST_MDEW_MDEVENT_2); \
  const MDEventWorkspace<MDEvent<3>, 3>::sptr CONST_MDEW_MDEVENT_3 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<3>, 3> >(workspace); \
  if (CONST_MDEW_MDEVENT_3) funcname<MDEvent<3>, 3>(CONST_MDEW_MDEVENT_3); \
  const MDEventWorkspace<MDEvent<4>, 4>::sptr CONST_MDEW_MDEVENT_4 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<4>, 4> >(workspace); \
  if (CONST_MDEW_MDEVENT_4) funcname<MDEvent<4>, 4>(CONST_MDEW_MDEVENT_4); \
  const MDEventWorkspace<MDEvent<5>, 5>::sptr CONST_MDEW_MDEVENT_5 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<5>, 5> >(workspace); \
  if (CONST_MDEW_MDEVENT_5) funcname<MDEvent<5>, 5>(CONST_MDEW_MDEVENT_5); \
  const MDEventWorkspace<MDEvent<6>, 6>::sptr CONST_MDEW_MDEVENT_6 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<6>, 6> >(workspace); \
  if (CONST_MDEW_MDEVENT_6) funcname<MDEvent<6>, 6>(CONST_MDEW_MDEVENT_6); \
  const MDEventWorkspace<MDEvent<7>, 7>::sptr CONST_MDEW_MDEVENT_7 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<7>, 7> >(workspace); \
  if (CONST_MDEW_MDEVENT_7) funcname<MDEvent<7>, 7>(CONST_MDEW_MDEVENT_7); \
  const MDEventWorkspace<MDEvent<8>, 8>::sptr CONST_MDEW_MDEVENT_8 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<8>, 8> >(workspace); \
  if (CONST_MDEW_MDEVENT_8) funcname<MDEvent<8>, 8>(CONST_MDEW_MDEVENT_8); \
  const MDEventWorkspace<MDEvent<9>, 9>::sptr CONST_MDEW_MDEVENT_9 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDEvent<9>, 9> >(workspace); \
  if (CONST_MDEW_MDEVENT_9) funcname<MDEvent<9>, 9>(CONST_MDEW_MDEVENT_9); \
  const MDEventWorkspace<MDLeanEvent<1>, 1>::sptr CONST_MDEW_MDLEANEVENT_1 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<1>, 1> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_1) funcname<MDLeanEvent<1>, 1>(CONST_MDEW_MDLEANEVENT_1); \
  const MDEventWorkspace<MDLeanEvent<2>, 2>::sptr CONST_MDEW_MDLEANEVENT_2 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<2>, 2> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_2) funcname<MDLeanEvent<2>, 2>(CONST_MDEW_MDLEANEVENT_2); \
  const MDEventWorkspace<MDLeanEvent<3>, 3>::sptr CONST_MDEW_MDLEANEVENT_3 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<3>, 3> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_3) funcname<MDLeanEvent<3>, 3>(CONST_MDEW_MDLEANEVENT_3); \
  const MDEventWorkspace<MDLeanEvent<4>, 4>::sptr CONST_MDEW_MDLEANEVENT_4 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<4>, 4> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_4) funcname<MDLeanEvent<4>, 4>(CONST_MDEW_MDLEANEVENT_4); \
  const MDEventWorkspace<MDLeanEvent<5>, 5>::sptr CONST_MDEW_MDLEANEVENT_5 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<5>, 5> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_5) funcname<MDLeanEvent<5>, 5>(CONST_MDEW_MDLEANEVENT_5); \
  const MDEventWorkspace<MDLeanEvent<6>, 6>::sptr CONST_MDEW_MDLEANEVENT_6 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<6>, 6> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_6) funcname<MDLeanEvent<6>, 6>(CONST_MDEW_MDLEANEVENT_6); \
  const MDEventWorkspace<MDLeanEvent<7>, 7>::sptr CONST_MDEW_MDLEANEVENT_7 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<7>, 7> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_7) funcname<MDLeanEvent<7>, 7>(CONST_MDEW_MDLEANEVENT_7); \
  const MDEventWorkspace<MDLeanEvent<8>, 8>::sptr CONST_MDEW_MDLEANEVENT_8 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<8>, 8> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_8) funcname<MDLeanEvent<8>, 8>(CONST_MDEW_MDLEANEVENT_8); \
  const MDEventWorkspace<MDLeanEvent<9>, 9>::sptr CONST_MDEW_MDLEANEVENT_9 = boost::dynamic_pointer_cast<const MDEventWorkspace<MDLeanEvent<9>, 9> >(workspace); \
  if (CONST_MDEW_MDLEANEVENT_9) funcname<MDLeanEvent<9>, 9>(CONST_MDEW_MDLEANEVENT_9); \
  } 
  



  // ------------- Typedefs for MDBox ------------------

  /// Typedef for a MDBox with 1 dimension 
  typedef MDBox<MDEvent<1>, 1> MDBox1;
  /// Typedef for a MDBox with 2 dimensions 
  typedef MDBox<MDEvent<2>, 2> MDBox2;
  /// Typedef for a MDBox with 3 dimensions 
  typedef MDBox<MDEvent<3>, 3> MDBox3;
  /// Typedef for a MDBox with 4 dimensions 
  typedef MDBox<MDEvent<4>, 4> MDBox4;
  /// Typedef for a MDBox with 5 dimensions 
  typedef MDBox<MDEvent<5>, 5> MDBox5;
  /// Typedef for a MDBox with 6 dimensions 
  typedef MDBox<MDEvent<6>, 6> MDBox6;
  /// Typedef for a MDBox with 7 dimensions 
  typedef MDBox<MDEvent<7>, 7> MDBox7;
  /// Typedef for a MDBox with 8 dimensions 
  typedef MDBox<MDEvent<8>, 8> MDBox8;
  /// Typedef for a MDBox with 9 dimensions 
  typedef MDBox<MDEvent<9>, 9> MDBox9;
  /// Typedef for a MDBox with 1 dimension 
  typedef MDBox<MDLeanEvent<1>, 1> MDBox1Lean;
  /// Typedef for a MDBox with 2 dimensions 
  typedef MDBox<MDLeanEvent<2>, 2> MDBox2Lean;
  /// Typedef for a MDBox with 3 dimensions 
  typedef MDBox<MDLeanEvent<3>, 3> MDBox3Lean;
  /// Typedef for a MDBox with 4 dimensions 
  typedef MDBox<MDLeanEvent<4>, 4> MDBox4Lean;
  /// Typedef for a MDBox with 5 dimensions 
  typedef MDBox<MDLeanEvent<5>, 5> MDBox5Lean;
  /// Typedef for a MDBox with 6 dimensions 
  typedef MDBox<MDLeanEvent<6>, 6> MDBox6Lean;
  /// Typedef for a MDBox with 7 dimensions 
  typedef MDBox<MDLeanEvent<7>, 7> MDBox7Lean;
  /// Typedef for a MDBox with 8 dimensions 
  typedef MDBox<MDLeanEvent<8>, 8> MDBox8Lean;
  /// Typedef for a MDBox with 9 dimensions 
  typedef MDBox<MDLeanEvent<9>, 9> MDBox9Lean;



  // ------------- Typedefs for IMDBox ------------------

  /// Typedef for a IMDBox with 1 dimension 
  typedef IMDBox<MDEvent<1>, 1> IMDBox1;
  /// Typedef for a IMDBox with 2 dimensions 
  typedef IMDBox<MDEvent<2>, 2> IMDBox2;
  /// Typedef for a IMDBox with 3 dimensions 
  typedef IMDBox<MDEvent<3>, 3> IMDBox3;
  /// Typedef for a IMDBox with 4 dimensions 
  typedef IMDBox<MDEvent<4>, 4> IMDBox4;
  /// Typedef for a IMDBox with 5 dimensions 
  typedef IMDBox<MDEvent<5>, 5> IMDBox5;
  /// Typedef for a IMDBox with 6 dimensions 
  typedef IMDBox<MDEvent<6>, 6> IMDBox6;
  /// Typedef for a IMDBox with 7 dimensions 
  typedef IMDBox<MDEvent<7>, 7> IMDBox7;
  /// Typedef for a IMDBox with 8 dimensions 
  typedef IMDBox<MDEvent<8>, 8> IMDBox8;
  /// Typedef for a IMDBox with 9 dimensions 
  typedef IMDBox<MDEvent<9>, 9> IMDBox9;
  /// Typedef for a IMDBox with 1 dimension 
  typedef IMDBox<MDLeanEvent<1>, 1> IMDBox1Lean;
  /// Typedef for a IMDBox with 2 dimensions 
  typedef IMDBox<MDLeanEvent<2>, 2> IMDBox2Lean;
  /// Typedef for a IMDBox with 3 dimensions 
  typedef IMDBox<MDLeanEvent<3>, 3> IMDBox3Lean;
  /// Typedef for a IMDBox with 4 dimensions 
  typedef IMDBox<MDLeanEvent<4>, 4> IMDBox4Lean;
  /// Typedef for a IMDBox with 5 dimensions 
  typedef IMDBox<MDLeanEvent<5>, 5> IMDBox5Lean;
  /// Typedef for a IMDBox with 6 dimensions 
  typedef IMDBox<MDLeanEvent<6>, 6> IMDBox6Lean;
  /// Typedef for a IMDBox with 7 dimensions 
  typedef IMDBox<MDLeanEvent<7>, 7> IMDBox7Lean;
  /// Typedef for a IMDBox with 8 dimensions 
  typedef IMDBox<MDLeanEvent<8>, 8> IMDBox8Lean;
  /// Typedef for a IMDBox with 9 dimensions 
  typedef IMDBox<MDLeanEvent<9>, 9> IMDBox9Lean;



  // ------------- Typedefs for MDGridBox ------------------

  /// Typedef for a MDGridBox with 1 dimension 
  typedef MDGridBox<MDEvent<1>, 1> MDGridBox1;
  /// Typedef for a MDGridBox with 2 dimensions 
  typedef MDGridBox<MDEvent<2>, 2> MDGridBox2;
  /// Typedef for a MDGridBox with 3 dimensions 
  typedef MDGridBox<MDEvent<3>, 3> MDGridBox3;
  /// Typedef for a MDGridBox with 4 dimensions 
  typedef MDGridBox<MDEvent<4>, 4> MDGridBox4;
  /// Typedef for a MDGridBox with 5 dimensions 
  typedef MDGridBox<MDEvent<5>, 5> MDGridBox5;
  /// Typedef for a MDGridBox with 6 dimensions 
  typedef MDGridBox<MDEvent<6>, 6> MDGridBox6;
  /// Typedef for a MDGridBox with 7 dimensions 
  typedef MDGridBox<MDEvent<7>, 7> MDGridBox7;
  /// Typedef for a MDGridBox with 8 dimensions 
  typedef MDGridBox<MDEvent<8>, 8> MDGridBox8;
  /// Typedef for a MDGridBox with 9 dimensions 
  typedef MDGridBox<MDEvent<9>, 9> MDGridBox9;
  /// Typedef for a MDGridBox with 1 dimension 
  typedef MDGridBox<MDLeanEvent<1>, 1> MDGridBox1Lean;
  /// Typedef for a MDGridBox with 2 dimensions 
  typedef MDGridBox<MDLeanEvent<2>, 2> MDGridBox2Lean;
  /// Typedef for a MDGridBox with 3 dimensions 
  typedef MDGridBox<MDLeanEvent<3>, 3> MDGridBox3Lean;
  /// Typedef for a MDGridBox with 4 dimensions 
  typedef MDGridBox<MDLeanEvent<4>, 4> MDGridBox4Lean;
  /// Typedef for a MDGridBox with 5 dimensions 
  typedef MDGridBox<MDLeanEvent<5>, 5> MDGridBox5Lean;
  /// Typedef for a MDGridBox with 6 dimensions 
  typedef MDGridBox<MDLeanEvent<6>, 6> MDGridBox6Lean;
  /// Typedef for a MDGridBox with 7 dimensions 
  typedef MDGridBox<MDLeanEvent<7>, 7> MDGridBox7Lean;
  /// Typedef for a MDGridBox with 8 dimensions 
  typedef MDGridBox<MDLeanEvent<8>, 8> MDGridBox8Lean;
  /// Typedef for a MDGridBox with 9 dimensions 
  typedef MDGridBox<MDLeanEvent<9>, 9> MDGridBox9Lean;



  // ------------- Typedefs for MDEventWorkspace ------------------

  /// Typedef for a MDEventWorkspace with 1 dimension 
  typedef MDEventWorkspace<MDEvent<1>, 1> MDEventWorkspace1;
  /// Typedef for a MDEventWorkspace with 2 dimensions 
  typedef MDEventWorkspace<MDEvent<2>, 2> MDEventWorkspace2;
  /// Typedef for a MDEventWorkspace with 3 dimensions 
  typedef MDEventWorkspace<MDEvent<3>, 3> MDEventWorkspace3;
  /// Typedef for a MDEventWorkspace with 4 dimensions 
  typedef MDEventWorkspace<MDEvent<4>, 4> MDEventWorkspace4;
  /// Typedef for a MDEventWorkspace with 5 dimensions 
  typedef MDEventWorkspace<MDEvent<5>, 5> MDEventWorkspace5;
  /// Typedef for a MDEventWorkspace with 6 dimensions 
  typedef MDEventWorkspace<MDEvent<6>, 6> MDEventWorkspace6;
  /// Typedef for a MDEventWorkspace with 7 dimensions 
  typedef MDEventWorkspace<MDEvent<7>, 7> MDEventWorkspace7;
  /// Typedef for a MDEventWorkspace with 8 dimensions 
  typedef MDEventWorkspace<MDEvent<8>, 8> MDEventWorkspace8;
  /// Typedef for a MDEventWorkspace with 9 dimensions 
  typedef MDEventWorkspace<MDEvent<9>, 9> MDEventWorkspace9;
  /// Typedef for a MDEventWorkspace with 1 dimension 
  typedef MDEventWorkspace<MDLeanEvent<1>, 1> MDEventWorkspace1Lean;
  /// Typedef for a MDEventWorkspace with 2 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<2>, 2> MDEventWorkspace2Lean;
  /// Typedef for a MDEventWorkspace with 3 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<3>, 3> MDEventWorkspace3Lean;
  /// Typedef for a MDEventWorkspace with 4 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<4>, 4> MDEventWorkspace4Lean;
  /// Typedef for a MDEventWorkspace with 5 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<5>, 5> MDEventWorkspace5Lean;
  /// Typedef for a MDEventWorkspace with 6 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<6>, 6> MDEventWorkspace6Lean;
  /// Typedef for a MDEventWorkspace with 7 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<7>, 7> MDEventWorkspace7Lean;
  /// Typedef for a MDEventWorkspace with 8 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<8>, 8> MDEventWorkspace8Lean;
  /// Typedef for a MDEventWorkspace with 9 dimensions 
  typedef MDEventWorkspace<MDLeanEvent<9>, 9> MDEventWorkspace9Lean;



  // ------------- Typedefs for MDBin ------------------

  /// Typedef for a MDBin with 1 dimension 
  typedef MDBin<MDEvent<1>, 1> MDBin1;
  /// Typedef for a MDBin with 2 dimensions 
  typedef MDBin<MDEvent<2>, 2> MDBin2;
  /// Typedef for a MDBin with 3 dimensions 
  typedef MDBin<MDEvent<3>, 3> MDBin3;
  /// Typedef for a MDBin with 4 dimensions 
  typedef MDBin<MDEvent<4>, 4> MDBin4;
  /// Typedef for a MDBin with 5 dimensions 
  typedef MDBin<MDEvent<5>, 5> MDBin5;
  /// Typedef for a MDBin with 6 dimensions 
  typedef MDBin<MDEvent<6>, 6> MDBin6;
  /// Typedef for a MDBin with 7 dimensions 
  typedef MDBin<MDEvent<7>, 7> MDBin7;
  /// Typedef for a MDBin with 8 dimensions 
  typedef MDBin<MDEvent<8>, 8> MDBin8;
  /// Typedef for a MDBin with 9 dimensions 
  typedef MDBin<MDEvent<9>, 9> MDBin9;
  /// Typedef for a MDBin with 1 dimension 
  typedef MDBin<MDLeanEvent<1>, 1> MDBin1Lean;
  /// Typedef for a MDBin with 2 dimensions 
  typedef MDBin<MDLeanEvent<2>, 2> MDBin2Lean;
  /// Typedef for a MDBin with 3 dimensions 
  typedef MDBin<MDLeanEvent<3>, 3> MDBin3Lean;
  /// Typedef for a MDBin with 4 dimensions 
  typedef MDBin<MDLeanEvent<4>, 4> MDBin4Lean;
  /// Typedef for a MDBin with 5 dimensions 
  typedef MDBin<MDLeanEvent<5>, 5> MDBin5Lean;
  /// Typedef for a MDBin with 6 dimensions 
  typedef MDBin<MDLeanEvent<6>, 6> MDBin6Lean;
  /// Typedef for a MDBin with 7 dimensions 
  typedef MDBin<MDLeanEvent<7>, 7> MDBin7Lean;
  /// Typedef for a MDBin with 8 dimensions 
  typedef MDBin<MDLeanEvent<8>, 8> MDBin8Lean;
  /// Typedef for a MDBin with 9 dimensions 
  typedef MDBin<MDLeanEvent<9>, 9> MDBin9Lean;


  //### END AUTO-GENERATED CODE ##################################################################

} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDEVENTFACTORY_H_ */





































