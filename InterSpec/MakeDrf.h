#ifndef MakeDrf_h
#define MakeDrf_h
/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InterSpec_config.h"

#include <vector>

#include <Wt/WContainerWidget>

#include "InterSpec/AuxWindow.h"
#include "InterSpec/MakeDrfFit.h"

class MakeDrf;
class PeakDef;
class InterSpec;
class MaterialDB;
class MakeDrfChart;

namespace Wt
{
  class WText;
  class WComboBox;
  class WLineEdit;
  class WCheckBox;
  class WDoubleSpinBox;
  class WSuggestionPopup;
}

class MakeDrfWindow : public AuxWindow
{
public:
  MakeDrfWindow( InterSpec *viewer, MaterialDB *materialDB, Wt::WSuggestionPopup *materialSuggest );
  
  virtual ~MakeDrfWindow();
  
protected:
  MakeDrf *m_makeDrf;
};//class MakeDrfWindow


class MakeDrf : public Wt::WContainerWidget
{
public:
  MakeDrf( InterSpec *viewer,
           MaterialDB *materialDB,
           Wt::WSuggestionPopup *materialSuggest,
           Wt::WContainerWidget *parent = nullptr );
  
  virtual ~MakeDrf();
  
protected:
  void handleSourcesUpdates();
  void handleShowFwhmPointsToggled();
  void chartEnergyRangeChangedCallback( double lower, double upper );
  
  void fitFwhmEqn( std::vector< std::shared_ptr<const PeakDef> > peaks,
                   const size_t num_gamma_channels );
  void updateFwhmEqn( std::vector<float> coefs, std::vector<float> uncerts,
                      const int functionalForm, //see DetectorPeakResponse::ResolutionFnctForm
                      const int fitid );
  
  void fitEffEqn( std::vector<MakeDrfFit::DetEffDataPoint> data );
  void updateEffEqn( std::vector<float> coefs, std::vector<float> uncerts, const int fitid );
  
  InterSpec *m_interspec;
  MaterialDB *m_materialDB;
  Wt::WSuggestionPopup *m_materialSuggest;
  
  MakeDrfChart *m_chart;
  
  Wt::WContainerWidget *m_files;
  
  Wt::WLineEdit *m_detDiameter;
  
  Wt::WCheckBox *m_showFwhmPoints;
  
  Wt::WComboBox *m_fwhmEqnType;
  
  Wt::WComboBox *m_effEqnOrder;
  
  Wt::WComboBox *m_effEqnUnits;
  
  /** ToDo: make chart properly interactive so user doesnt need to input the
   energy range manually.
   */
  Wt::WDoubleSpinBox *m_chartLowerE;
  Wt::WDoubleSpinBox *m_chartUpperE;
  
  Wt::WText *m_errorMsg;
  
  /** Identifies current fit for FWHM.  Fits are done in an auxilary thread,
   and the user may change things before the fit is done, and some fits take a
   lot longer than others, so to make sure the the correct one is displayed,
   we will use this variable (which is only touched from main Wt thread).
   */
  int m_fwhmFitId;
  int m_effEqnFitId;
  
  std::vector<float> m_fwhmCoefs, m_fwhmCoefUncerts;
  std::vector<float> m_effEqnCoefs, m_effEqnCoefUncerts;
};//class MakeDrf

#endif //MakeDrf_h