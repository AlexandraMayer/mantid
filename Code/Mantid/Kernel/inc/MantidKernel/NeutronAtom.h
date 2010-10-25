/*
 * NeutronAtom.h
 *
 *  Created on: Oct 21, 2010
 *      Author: pf9
 */

#ifndef NEUTRONATOM_H_
#define NEUTRONATOM_H_

#include <string>
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

/**
 * Structure to store neutronic scatting information for the various elements.
 * This is taken from http://www.ncnr.nist.gov/resources/n-lengths/list.html.
 */
struct NeutronAtom {
  NeutronAtom(const std::string &symbol, const uint8_t z,
              const double coh_b_real, const double inc_b_real,
              const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const std::string &symbol,
              const uint8_t z, const uint8_t a,
              const double coh_b_real, const double inc_b_real,
              const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const std::string &symbol,
              const uint8_t z, const uint8_t a,
              const double coh_b_real, const double coh_b_img,
              const double inc_b_real, const double inc_b_img,
              const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const std::string &symbol, const std::string &alias,
              const uint8_t z, const uint8_t a,
              const double coh_b_real, const double inc_b_real,
              const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  /** The atomic symbol. */
  std::string symbol;

  /** Other names that this atom is known by, such as D for deuterium. */
  std::string alias;

  /** The atomic number, or number of protons, for the atom. */
  uint8_t z_number;

  /**
   * The total number of protons and neutrons, or mass number, for the atom.
   * For isotopic averages this is set to zero.
   */
  uint8_t a_number;

  /** The real part of the coherent scattering length in fm. */
  double coh_scatt_length_real;

  /** The imaginary part of the coherent scattering length in fm. */
  double coh_scatt_length_img;

  /** The real part of the incoherent scattering length in fm. */
  double inc_scatt_length_real;

  /** The imaginary part of the incoherent scattering length in fm. */
  double inc_scatt_length_img;

  /** The coherent scattering cross section in barns. */
  double coh_scatt_xs;

  /** The incoherent scattering cross section in barns. */
  double inc_scatt_xs;

  /** The total scattering cross section in barns. */
  double tot_scatt_xs;

  /** The absorption cross section for 2200m/s neutrons in barns. */
  double abs_scatt_xs;
};

} //Namespace Kernel
} //Namespace Mantid

#endif /* NEUTRONATOM_H_ */
