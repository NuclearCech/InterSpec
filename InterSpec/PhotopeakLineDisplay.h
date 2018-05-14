#ifndef PhotopeakLineDisplay_h
#define PhotopeakLineDisplay_h
/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov, or srb@sandia.gov.
 
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

#include <string>
#include <vector>

#include <Wt/WContainerWidget>
#include <Wt/WAbstractItemModel>

#include "InterSpec/ReactionGamma.h"
#include "sandia_decay/SandiaDecay.h"
#include "InterSpec/ReferenceLineInfo.h"

class MaterialDB;
class SpectrumChart;
class ShieldingSelect;

namespace SandiaDecay
{
  struct Nuclide;
  struct Element;
}

namespace rapidxml
{
  template<class Ch> class xml_node;
  template<class Ch> class xml_document;
}//namespace rapidxml

namespace Wt
{
  class WText;
  class WCheckBox;
  class WLineEdit;
  class WTreeView;
  class WGridLayout;
  class WPushButton;
  class WDoubleSpinBox;
  class WSuggestionPopup;
}//namespace Wt

class InterSpec;
class DetectorDisplay;
#if ( USE_SPECTRUM_CHART_D3 )
class D3SpectrumDisplayDiv;
#else
class SpectrumDisplayDiv;
#endif
class IsotopeNameFilterModel;


class DecayParticleModel : public  Wt::WAbstractItemModel
{
  //Model to display

public:
  enum Column
  { kEnergy, kBranchingRatio, kResponsibleNuc,
    kDecayMode, kParticleType, kNumColumn
  };//enum Column

  struct RowData
  {
    static const int XRayDecayMode;
    static const int ReactionToGammaMode;
    static const int NormGammaDecayMode;

    float energy, branchRatio;
    int decayMode;
    SandiaDecay::ProductType particle;
    const SandiaDecay::Nuclide *responsibleNuc;
  };//struct RowData

  static bool less_than( const RowData &lhs, const RowData &rhs,
                         const Column r, const Wt::SortOrder order );

  DecayParticleModel( Wt::WObject *parent = 0 );
  virtual ~DecayParticleModel();

  virtual Wt::WFlags<Wt::ItemFlag> flags( const Wt::WModelIndex &index ) const;
  virtual boost::any data( const Wt::WModelIndex &index,
                                            int role = Wt::DisplayRole ) const;
  boost::any headerData( int section,
                         Wt::Orientation orientation = Wt::Horizontal,
                         int role = Wt::DisplayRole ) const;
  virtual int columnCount( const Wt::WModelIndex &parent
                                                    = Wt::WModelIndex() ) const;
  virtual int rowCount( const Wt::WModelIndex &parent = Wt::WModelIndex() ) const;
  virtual Wt::WModelIndex parent( const Wt::WModelIndex &index) const;
  virtual Wt::WModelIndex index( int row, int column,
                      const Wt::WModelIndex &parent = Wt::WModelIndex() ) const;
  virtual void sort( int column, Wt::SortOrder order = Wt::AscendingOrder );
  void clear();
  virtual void setRowData( const std::vector<RowData> &newData );

protected:
  Column m_sortColumn;
  Wt::SortOrder m_sortOrder;
  std::vector<RowData> m_data;
};//class DecayParticleModel(...)



class PhotopeakLineDisplay : public Wt::WContainerWidget
{
public:
  PhotopeakLineDisplay(
#if ( USE_SPECTRUM_CHART_D3 )
                        D3SpectrumDisplayDiv *chart,
#else
                        SpectrumDisplayDiv *chart,
#endif
                        MaterialDB *materialDB,
                        Wt::WSuggestionPopup *materialSuggest,
                        InterSpec *specViewer,
                        Wt::WContainerWidget *parent = 0 );
  virtual ~PhotopeakLineDisplay();

  void setFocusToIsotopeEdit();

  //clearAllLines(): removes lines from both this widget (table and entry form),
  //  as well as on the chart.
  void clearAllLines();

  //fitPeaks(): attempts to fit and add the peaks fir the currently showing
  //  nuclide
  void fitPeaks();
  
  //setIsotope(...): sets the nuclde to specified nuc and age; if age is less
  //  than zero, then current age will not be modified
  void setIsotope( const SandiaDecay::Nuclide *nuc, double age = -1.0 );
  
  //setElement(...): sets to display xrays for the desired element
  void setElement( const SandiaDecay::Element *element );
    
  //setReaction(...): sets to display gammas for the desired reaction
  void setReaction( const ReactionGamma::Reaction *rctn );
  
  //setNuclideAndAge(...): sets the widgets nuclide text to passed in value.
  //  If length of age is non-zero, then will set the age string, otherwise
  //  will defer to auto age setting.
  //  'name' is assumed to be a valid source name, and is not checked.
  void setNuclideAndAge( const std::string &name, const std::string &age );
  
  //setShieldingMaterialAndThickness(...): sets the shielding material as well
  //  as thickness.  The name must be either in the material DB, or a chemical
  //  formula (which if so, will be added to the list of user suggestions for
  //  the current session).  The thickness must be accepted by
  //  PhysicalUnits::stringToDistance(...).  Throws exception if either field
  //  is invalid.
  void setShieldingMaterialAndThickness( const std::string &name,
                                         const std::string &thickness );
  
  //currentlyShowingNuclide(): as its name implies; returns NULL if not showing
  //  a nuclide.
  const ReferenceLineInfo &currentlyShowingNuclide() const;

  //persistedNuclides(): nuclides the user as asked to be persisted, and are
  //  showing.  Does not include currentlyShowingNuclide(), however the user
  //  may have previously persisted the same nuclide as
  //  currentlyShowingNuclide() so a nuclide could be in both places.
  const std::vector<ReferenceLineInfo> &persistedNuclides() const;

  //serialize(...): Serializes the current state of this widget to the XML
  //  document or string.  Currently, all persisted lines will be assumed to
  //  have the same shielding as the currently saved lines.
  void serialize( std::string &xml_data  ) const;
  void serialize( rapidxml::xml_node<char> *parent_node ) const;

  //deSerialize(...): de-serializes the widget from the XML string created by
  //  serialize(...).
  void deSerialize( std::string &xml_data  );
  
  //persistCurentLines(): persists current lines
  void persistCurentLines();
  
protected:
  void updateDisplayChange();
  void handleIsotopeChange( const bool useCurrentAge );

  //refreshLinesDisplayedToGui(): makes setting and re-sends to client the lines
  //  that should be displayed, based on m_currentlyShowingNuclide and
  //  m_persisted objects
  void refreshLinesDisplayedToGui( int millisecdelay );
  
#if ( USE_SPECTRUM_CHART_D3 )
  D3SpectrumDisplayDiv *m_chart;
#else
  SpectrumDisplayDiv *m_chart;
#endif
  InterSpec *m_spectrumViewer;
  
  Wt::WLineEdit *m_nuclideEdit;
  Wt::WSuggestionPopup *m_nuclideSuggest;

  Wt::WLineEdit *m_ageEdit;
  Wt::WDoubleSpinBox *m_lowerBrCuttoff;

  Wt::WCheckBox *m_showLines;
  Wt::WCheckBox *m_promptLinesOnly;

  Wt::WText *m_halflife;
    
  Wt::WPushButton *m_persistLines;
  Wt::WPushButton *m_clearLines;
  //Wt::WPushButton *m_fitPeaks;

  Wt::WCheckBox *m_showGammas;
  Wt::WCheckBox *m_showXrays;
  Wt::WCheckBox *m_showAlphas;
  Wt::WCheckBox *m_showBetas;


  DetectorDisplay *m_detectorDisplay;
  MaterialDB *m_materialDB;                 //not owned by this object
  Wt::WSuggestionPopup *m_materialSuggest;  //not owned by this object
  ShieldingSelect *m_shieldingSelect;

  Wt::WTreeView *m_particleView;
  DecayParticleModel *m_particleModel;

  Wt::WGridLayout *m_layout;

  std::vector<ReferenceLineInfo> m_persisted;
  ReferenceLineInfo m_currentlyShowingNuclide;
  
  static const int sm_xmlSerializationVersion;
};//class PhotopeakLineDisplay


#endif  //ifndef PhotopeakLineDisplay_h
