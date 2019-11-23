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

// Disable streamsize <=> size_t warnings in boost
#pragma warning(disable:4244)

#include <memory>
#include <vector>
#include <iostream>
#include <stdexcept>

#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WImage>
#include <Wt/WLabel>
#include <Wt/WAnchor>
#include <Wt/WServer>
#include <Wt/WSvgImage>
#include <Wt/WComboBox>
#include <Wt/WLineEdit>
#include <Wt/WTableCell>
#include <Wt/WTabWidget>
#include <Wt/WPushButton>
#include <Wt/WGridLayout>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WItemDelegate>
#include <Wt/WDoubleSpinBox>

#include "InterSpec/PeakDef.h"
#include "InterSpec/PeakFit.h"
#include "InterSpec/SpecMeas.h"
#include "InterSpec/PeakModel.h"
#include "InterSpec/AuxWindow.h"
#include "InterSpec/HelpSystem.h"
#include "InterSpec/ColorSelect.h"
#include "InterSpec/InterSpecApp.h"
#include "InterSpec/WarningWidget.h"
#include "InterSpec/PeakInfoDisplay.h"
#include "SpecUtils/UtilityFunctions.h"
#include "InterSpec/RowStretchTreeView.h"
#include "InterSpec/PeakSearchGuiUtils.h"
#if( USE_SPECTRUM_CHART_D3 )
#include "InterSpec/D3SpectrumDisplayDiv.h"
#else
#include "InterSpec/SpectrumDisplayDiv.h"
#endif
#include "InterSpec/DetectorPeakResponse.h"
#include "InterSpec/IsotopeSelectionAids.h"
#include "InterSpec/ShieldingSourceDisplay.h"


using namespace Wt;
using namespace std;

namespace
{
#if( ALLOW_PEAK_COLOR_DELEGATE )
  class ColorDelegate : public Wt::WAbstractItemDelegate
  {
  public:
    class EditWidget : public Wt::WContainerWidget
    {
    public:
      EditWidget( const Wt::WModelIndex& index,
                 const Wt::WFlags<Wt::ViewItemRenderFlag> flags,
                 ColorDelegate *parent )
      {
        m_color = nullptr;
        m_parent = parent;
        
        auto model = dynamic_cast<const PeakModel *>( index.model() );
        m_peakModel = const_cast<PeakModel *>(model);
        
        if( model )
        {
          try
          {
            m_origPeak = model->peak( index );
            if( m_origPeak )
              m_origColor = m_origPeak->lineColor();
          }catch(...){}
        }
        updateRender( index, flags );
      }//EditWidget constructor
      
      virtual ~EditWidget(){}
      
      WString cssColor()
      {
        if( m_color && !m_color->valueText().empty() )
          return m_color->valueText();
        else
          return "none";
      }//cssColor()
      
      void setCssCollor( const WString &s )
      {
        cout << "setCssCollor: " << s.toUTF8() << endl;
        const string ss = s.toUTF8();
        if( m_color )
          m_color->setValueText( (ss.empty() || ss=="none") ? "" : s );
        else
        {
          if( ss.empty() || ss=="none" )
            setAttributeValue( "style", "margin: 5px; background-color: null; border: 1px #e1e1e1" );
          else
            setAttributeValue( "style", "margin: 5px; border: 1px #e1e1e1; background-color: " + ss );
        }
      }//void setCssCollor( WString &s )
      
      void closeEditor( bool save )
      {
        auto index = m_peakModel->indexOfPeak(m_origPeak);
        
        WString color = m_origColor;
        if( index.isValid() && save && m_color && m_color->valueText().toUTF8()!="#ebebeb" )
          color = m_color->valueText();
        
        m_peakModel->setData( index, color );
        if( m_color )
          delete m_color;
        m_color = nullptr;
        updateRender(index,0);
      }
      
      void colorSelected( const std::string &color )
      {
        closeEditor( color!=m_origColor.toUTF8() );
      }
      
      void updateRender( const Wt::WModelIndex &index,
                        const Wt::WFlags<Wt::ViewItemRenderFlag> flags )
      {
        cout << "updateRender: flags=" << flags.value() << endl;
        
        auto model = dynamic_cast<const PeakModel *>( index.model() );
        
        if( model && (flags & RenderEditing) ) //RenderSelected, RenderEditing, RenderFocused, RenderInvalid
        {
          if( !m_color )
          {
            m_color = new ColorSelect(ColorSelect::PrefferNative,this);
            setAttributeValue( "style", "margin: 0px; background-color: null;" );
            m_color->setAttributeValue("style", "height: 15px;" );
            m_color->cssColorChanged().connect( boost::bind( &EditWidget::colorSelected, this, _1 ) );
          }
          try
          {
            const auto &p = model->peak(index);
            m_color->setValueText( p->lineColor().empty() ? "#ebebeb" : p->lineColor().c_str() );
          }catch(...)
          {
          }
        }else
        {
          try
          {
            if( !model )
              throw runtime_error("");
            
            const auto &p = model->peak( index );
            if( p->lineColor().empty() )
              setAttributeValue( "style", "margin: 5px; background-color: null; border: 1px #e1e1e1" );
            else
              setAttributeValue( "style", "margin: 5px; border: 1px #e1e1e1; background-color: " + p->lineColor() );
          }catch(...)
          {
            setAttributeValue( "style", "margin: 5px; border: 1px #e1e1e1; background-color: grey" );
          }
        }
      }//void updateRender(...)
      
    protected:
      Wt::WString m_origColor;
      PeakModel::PeakShrdPtr m_origPeak;
      PeakModel *m_peakModel;
      ColorSelect *m_color;
      ColorDelegate *m_parent;
    };//class EditWidget
    
  public:
    ColorDelegate( Wt::WObject *parent = NULL )
      : WAbstractItemDelegate(parent)
    {
      closeEditor().connect( boost::bind( &ColorDelegate::doCloseEditor, this, _1, _2 ) );
    }
    
    virtual ~ColorDelegate(){}
    
    virtual Wt::WWidget *update( Wt::WWidget *widget,
                                const Wt::WModelIndex &index,
                                Wt::WFlags< Wt::ViewItemRenderFlag > flags )
    {
      EditWidget *edit = dynamic_cast<EditWidget *>(widget);
      if( !edit )
        edit = new EditWidget( index, flags, this );
      else
        edit->updateRender( index, flags );
      
      return edit;
    }//WWidget *update(...)
    
    
    void doCloseEditor( WWidget *editor, bool save )
    {
      EditWidget *edit = dynamic_cast<EditWidget *>(editor);
      if( edit )
        edit->closeEditor(save);
    }//void doCloseEditor( WWidget *editor, bool save )
    
  protected:
    
 
    boost::any editState( Wt::WWidget *editor ) const
    {
      EditWidget *w = dynamic_cast<EditWidget *>(editor);
      if( !w )
      {
        cerr << SRC_LOCATION << "\n\tLogic error - fix me!" << endl;
        return boost::any();
      }//if( !w )
      
      WString csscolor = "none";
      if( w )
        csscolor = w->cssColor();
      
      return boost::any( csscolor );
    }//boost::any editState( WWidget *editor ) const

    
    void setEditState( Wt::WWidget *editor, const boost::any &value ) const
    {
      EditWidget *w = dynamic_cast<EditWidget *>(editor);
      if( !w )
      {
        cerr << SRC_LOCATION << "\n\tLogic error - fix me!" << endl;
        return;
      }
      
      try
      {
        w->setCssCollor( boost::any_cast<WString>(value) );
      }catch(...)
      {
        cerr << SRC_LOCATION << "\n\tPossible Logic error - fix me!" << endl;
      }//try / catch
    }//void setEditState( WWidget *editor, const boost::any& value ) const

    
    void setModelData( const boost::any &editState,
                      Wt::WAbstractItemModel *model,
                      const Wt::WModelIndex &index ) const
    {
      if( model )
        model->setData( index, editState, EditRole );
    }//void setModelData(...)
  };//class ColorDelegate
#endif //ALLOW_PEAK_COLOR_DELEGATE
}//namespace


PeakInfoDisplay::PeakInfoDisplay( InterSpec *viewer,
#if ( USE_SPECTRUM_CHART_D3 )
                                  D3SpectrumDisplayDiv *spectrumDisplayDiv,
#else
                                  SpectrumDisplayDiv *spectrumDisplayDiv,
#endif
                                  PeakModel *peakModel,
                                  Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_model( peakModel ),
    m_viewer( viewer ),
    m_spectrumDisplayDiv( spectrumDisplayDiv ),
    m_infoLayout( NULL ),
    m_infoView( NULL ),
    m_deletePeak( NULL ),
    m_searchForPeaks( NULL )
{
  assert( m_spectrumDisplayDiv );
  addStyleClass( "PeakInfoDisplay" );
  init();
//  setLayoutSizeAware(true);
}//PeakInfoDisplay constructor


PeakInfoDisplay::~PeakInfoDisplay()
{
}//~PeakInfoDisplay()


void PeakInfoDisplay::confirmRemoveAllPeaks()
{
  if( m_model->rowCount() <= 0 )
    return;
  
  AuxWindow *window = new AuxWindow( "Confirmation",
              (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::PhoneModal | AuxWindowProperties::DisableCollapse) );
  window->rejectWhenEscapePressed();
  WText * text = new WText("Erase All Peaks?");
  window->stretcher()->addWidget(text,0,0);
  
  auto foot = window->footer();
  
  WPushButton *yes_button = nullptr, *no_button = nullptr;
  
  if( m_viewer->isMobile() )
  {
    window->titleBar()->hide();
    yes_button = new WPushButton( "Yes" );
    no_button = new WPushButton( "No" );
    yes_button->setWidth( WLength(50,WLength::Percentage) );
    no_button->setWidth( WLength(50,WLength::Percentage) );
    yes_button->setStyleClass( "ModalConfirmBtn" );
    no_button->setStyleClass( "ModalConfirmBtn" );
    
    foot->addWidget( yes_button );
    foot->addWidget( no_button );
    foot->addStyleClass( "MobileConfirmFooter" );
  }else
  {
    yes_button = new WPushButton("Yes");
    foot->addWidget( yes_button );
    yes_button->setFloatSide(Wt::Right);
    no_button = window->addCloseButtonToFooter("No");
  }//if( m_viewer->isMobile() )
  
  no_button->clicked().connect( window, &AuxWindow::hide );
  yes_button->clicked().connect( window, &AuxWindow::hide );
#if ( USE_SPECTRUM_CHART_D3 )
  yes_button->clicked().connect( boost::bind( &D3SpectrumDisplayDiv::removeAllPeaks, m_spectrumDisplayDiv ) );
#else
  yes_button->clicked().connect( boost::bind( &PeakModel::removeAllPeaks, m_model ) );
#endif
  
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  window->show();
  window->setMinimumSize(200, WLength::Auto);
  window->centerWindow();
}//void confirmRemoveAllPeaks()


void PeakInfoDisplay::assignNuclidesFromRefLines()
{
  PeakSearchGuiUtils::assign_peak_nuclides_from_reference_lines( m_viewer );
}//void assignNuclidesFromRefLines()


void PeakInfoDisplay::enablePeakSearchButton( bool enable )
{
  m_searchForPeaks->setEnabled( enable );
}//void enablePeakSearchButton( bool enable = true )



void PeakInfoDisplay::createNewPeak()
{
  std::shared_ptr<const Measurement> meas = m_viewer->displayedHistogram(SpectrumType::kForeground);
  
  if( !meas || meas->num_gamma_channels() < 7 )
  {
    passMessage( "A foreground spectrum must be loaded to add a peak.",
                "", WarningWidget::WarningMsgHigh );
    return;
  }//if( we dont have a valid foreground )
  
  float xmin = static_cast<float>( m_spectrumDisplayDiv->xAxisMinimum() );
  float xmax = static_cast<float>( m_spectrumDisplayDiv->xAxisMaximum() );

  const size_t nbin = meas->num_gamma_channels();
  xmin = std::max( xmin, meas->gamma_channel_lower(0) );
  xmax = std::min( xmax, meas->gamma_channel_upper(nbin-1) );
  
  const float minfwhm = 0.05f, maxfwhm = 450.0f;  //reasonable range of peak widths
  
  //To get the width,
  //  1) see if there is a peak above and below the current one, if so interpolate
  //  2) see if DRF contains FWHM
  //  3) see if there is a peak above or below, and scale by sqrt
  //  4) Guess based on number of bins.
  auto estimateFWHM = [this,nbin,minfwhm,maxfwhm]( const float energy ) -> float {
    const auto peaks = m_model->peakVec();
    const auto lb = std::lower_bound( begin(peaks), end(peaks), PeakDef(energy,1.0,1.0), &PeakDef::lessThanByMean );
    const auto ub = lb==end(peaks) ? end(peaks) : lb + 1;
    if( lb!=end(peaks) && ub!=end(peaks) && lb->gausPeak() && ub->gausPeak() )
      return static_cast<float>( lb->fwhm() + (ub->fwhm() - lb->fwhm())*(energy - lb->mean()) / (ub->mean() - lb->mean()) );
    
    std::shared_ptr<SpecMeas> specmeas = m_viewer->measurment(SpectrumType::kForeground);
    std::shared_ptr<DetectorPeakResponse> drf = specmeas ? specmeas->detector() : nullptr;
    if( drf && drf->hasResolutionInfo() )
      return std::min( maxfwhm, std::max(minfwhm,drf->peakResolutionFWHM(energy)) );
    
    if( peaks.empty() )
    {
      try
      {
        const string datadir = InterSpec::staticDataDirectory();
        string drf_dir = UtilityFunctions::append_path(datadir, "GenericGadrasDetectors/HPGe 40%" );
        if( nbin <= 2049 )
          drf_dir = UtilityFunctions::append_path(datadir, "GenericGadrasDetectors/NaI 1x1" );
        
        drf = make_shared<DetectorPeakResponse>();
        drf->fromGadrasDirectory( drf_dir );
        
        return std::min( maxfwhm, std::max(minfwhm,drf->peakResolutionFWHM(energy)) );
      }catch(...)
      {
        if( nbin <= 2049 )
          return std::min( maxfwhm, std::max(minfwhm,2.634f*17.5f*sqrt(energy/661.0f)) );
        return std::min( maxfwhm, std::max(minfwhm,2.634f*0.67f*sqrt(energy/661.0f)) );
      }
    }//if( peaks.empty() )
  
    const PeakDef &refpeak = (lb!=end(peaks) ? *lb : (peaks.front().mean() > energy) ? peaks.front() : peaks.back());
    const double ref_width = refpeak.gausPeak() ? refpeak.fwhm() : 0.25f*refpeak.roiWidth();
    const double ref_energy = refpeak.gausPeak() ? refpeak.mean() : 0.5*(refpeak.upperX() + refpeak.lowerX());
    
    return std::min( maxfwhm, std::max(minfwhm,static_cast<float>( ref_width*sqrt(energy/ref_energy) )) );
  };//estimateFWHM lambda
  
  
  // Now create a dialog where user can specify the mean, sigma, roi range, and roi polynomial order...
  const float initialEnergy = 0.5f*(xmin + xmax);
  const float initialFWHM = estimateFWHM(initialEnergy);
  const double initialArea = std::max(10.0,0.2*meas->gamma_integral(initialEnergy-initialFWHM, initialEnergy+initialFWHM));
  
  shared_ptr<PeakDef> candidatePeak = make_shared<PeakDef>(initialEnergy,initialFWHM/2.634,initialArea);
  
  int roiLowBin, roiHighBin;
  estimatePeakFitRange( *candidatePeak, meas, roiLowBin, roiHighBin );
  //I think we are garunteed the bins to be in range, but we'll enforce this JIC
  const size_t roiLowerChannel = static_cast<size_t>( std::max(roiLowBin,1) );
  const size_t roiUpperChannel = std::min( static_cast<size_t>(roiHighBin), meas->num_gamma_channels()-1 );
  
  const double initialRoiLower = meas->gamma_channel_lower( roiLowerChannel );
  const double initialRoiUpper = meas->gamma_channel_upper( roiUpperChannel );
  candidatePeak->continuum()->setRange( initialRoiLower, initialRoiUpper );
  
  const float minEnergy = meas->gamma_channel_lower(0);
  const float maxEnergy = meas->gamma_channel_upper(nbin-1);
  
  AuxWindow *window = new AuxWindow( "Add Peak",
                                    (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::PhoneModal | AuxWindowProperties::DisableCollapse) );
  window->rejectWhenEscapePressed();
  
  WTable *table = new WTable( window->contents() );
  
  WLabel *label = new WLabel( "Peak Mean", table->elementAt(0,0) );
  
  WDoubleSpinBox *energySB = new WDoubleSpinBox( table->elementAt(0,1) );
  //energySB->setSuffix( "keV" );
  energySB->setRange( minEnergy, maxEnergy );
  energySB->setValue( initialEnergy );
  
  
  label = new WLabel( "Peak FWHM", table->elementAt(1,0) );
  WDoubleSpinBox *fwhmSB = new WDoubleSpinBox( table->elementAt(1,1) );
  //fwhmSB->setSuffix( "keV" );
  fwhmSB->setRange( meas->gamma_channel_lower(0), meas->gamma_channel_upper(nbin-1) );
  fwhmSB->setRange( minfwhm, maxfwhm );
  fwhmSB->setValue( initialFWHM );
  

  label = new WLabel( "Peak Amp.", table->elementAt(2,0) );
  WDoubleSpinBox *areaSB = new WDoubleSpinBox( table->elementAt(2,1) );
  areaSB->setRange( 0, meas->gamma_channels_sum(0, nbin-1) );
  areaSB->setValue( initialArea );
  
  label = new WLabel( "ROI Lower", table->elementAt(3,0) );
  WDoubleSpinBox *roiLowerSB = new WDoubleSpinBox( table->elementAt(3,1) );
  roiLowerSB->setRange( minEnergy, maxEnergy );
  roiLowerSB->setValue( initialRoiLower );
  
  label = new WLabel( "ROI Upper", table->elementAt(4,0) );
  WDoubleSpinBox *roiUpperSB = new WDoubleSpinBox( table->elementAt(4,1) );
  roiUpperSB->setRange( minEnergy, maxEnergy );
  roiUpperSB->setValue( initialRoiUpper );
  
  
  label = new WLabel( "Continuum", table->elementAt(5,0) );
  WComboBox *contType = new WComboBox( table->elementAt(5,1) );
  contType->addItem( "None" );
  contType->addItem( "Constant" );
  contType->addItem( "Linear" );
  contType->addItem( "Quadratic" );
  contType->addItem( "Global Cont." );
  contType->setCurrentIndex( 2 );
  
  WText *chart = new WText( "", Wt::XHTMLUnsafeText, window->contents() );
  //chart->addStyleClass( "DrfPeakChart" );
  
  //Make sepearte lambdas for changing each quantitiy
  
  auto updateCandidatePeakPreview = [=](){
    
    std::shared_ptr<const Measurement> meas = m_viewer->displayedHistogram(SpectrumType::kForeground);
    
    if( !meas )
    {
      chart->setText( "No foreground spectrum" );
      return;
    }

    auto peaks = std::make_shared< std::deque<std::shared_ptr<const PeakDef> > >();
    peaks->push_back( candidatePeak );
      
    const bool compact = false;
    const int width_px = 500;
    const int height_px = 350;
    const double roiWidth = candidatePeak->upperX() - candidatePeak->lowerX();
    const double lower_energy = candidatePeak->lowerX() - roiWidth;
    const double upper_energy = candidatePeak->upperX() + roiWidth;
    std::shared_ptr<const ColorTheme> theme = m_viewer->getColorTheme();
    const std::vector<std::shared_ptr<const ReferenceLineInfo>> reflines;
      
    std::shared_ptr<Wt::WSvgImage> preview
        = PeakSearchGuiUtils::renderChartToSvg( meas, peaks, reflines, lower_energy,
                                                upper_energy, width_px, height_px, theme, compact );
      
    if( preview )
    {
      stringstream strm;
      preview->write( strm );
      chart->setText( strm.str() );
    }else
    {
      chart->setText( "Error rendering preview" );
    }

  };//updateCandidatePeak lambda

  auto roiTypeChanged = [=](){
    switch( contType->currentIndex() )
    {
      case 0: //None
        candidatePeak->continuum()->setType( PeakContinuum::OffsetType::NoOffset );
        break;
        
      case 1:  //constant
        candidatePeak->continuum()->calc_linear_continuum_eqn( meas, candidatePeak->lowerX(), candidatePeak->upperX(), 2 );
        candidatePeak->continuum()->setType( PeakContinuum::OffsetType::Constant );
        break;
        
      case 2: //linear
        candidatePeak->continuum()->calc_linear_continuum_eqn( meas, candidatePeak->lowerX(), candidatePeak->upperX(), 2 );
        candidatePeak->continuum()->setType( PeakContinuum::OffsetType::Linear );
        break;
        
      case 3: //quadratic
        candidatePeak->continuum()->calc_linear_continuum_eqn( meas, candidatePeak->lowerX(), candidatePeak->upperX(), 2 );
        candidatePeak->continuum()->setType( PeakContinuum::OffsetType::Quadratic );
        break;
        
      case 4: //global
      {
        std::shared_ptr<const Measurement> gcontinuum = candidatePeak->continuum()->externalContinuum();
        if( !gcontinuum )
          gcontinuum = estimateContinuum( meas );
        candidatePeak->continuum()->setType( PeakContinuum::OffsetType::External );
        candidatePeak->continuum()->setExternalContinuum( gcontinuum );
      }
        break;
    }//switch( contType->currentIndex() )
    
    updateCandidatePeakPreview();
  };//roiTypeChanged
  
  
  auto meanChanged = [=](){
    const double oldmean = candidatePeak->mean();
    
    if( WValidator::State::Valid != energySB->validate() )
      energySB->setValue( oldmean );
    
    const double newmean = energySB->value();
    const double change = newmean - oldmean;
    
    const double newLowerX = candidatePeak->lowerX() + change;
    const double newUpperX = candidatePeak->upperX() + change;
    roiLowerSB->setValue( newLowerX );
    roiUpperSB->setValue( newUpperX );
    
    candidatePeak->setMean( newmean );
    candidatePeak->continuum()->setRange( newLowerX, newUpperX );
    roiTypeChanged();
  };//meanChanged
  
  auto fwhmChanged = [=](){
    const double oldFWHM = candidatePeak->fwhm();
    
    if( WValidator::State::Valid != fwhmSB->validate() )
      candidatePeak->setSigma( oldFWHM/2.634 );
    
    const double newFWHM = fwhmSB->value();
    candidatePeak->setSigma( newFWHM/2.634 );
    
    updateCandidatePeakPreview();
  };
  
  auto ampChanged = [=](){
    const double oldAmp = candidatePeak->amplitude();
    
    if( WValidator::State::Valid != areaSB->validate() )
      areaSB->setValue( oldAmp );
    
    const double newAmp = areaSB->value();
    candidatePeak->setAmplitude( newAmp );
    
    updateCandidatePeakPreview();
  };

  
  auto roiRangeChanged = [=](){
    if( WValidator::State::Valid != roiLowerSB->validate() )
      roiLowerSB->setValue( candidatePeak->lowerX() );
    
    if( WValidator::State::Valid != roiUpperSB->validate() )
      roiUpperSB->setValue( candidatePeak->upperX() );
    
    double roiLower = roiLowerSB->value();
    double roiUpper = roiUpperSB->value();
    if( roiLower >= roiUpper )
    {
      roiLowerSB->setValue( roiUpper );
      roiUpperSB->setValue( roiLower );
      std::swap( roiLower, roiUpper );
    }
    
    if( candidatePeak->mean() < roiLower )
    {
      energySB->setValue( roiLower );
      candidatePeak->setMean( roiLower );
    }
    
    if( candidatePeak->mean() > roiUpper )
    {
      energySB->setValue( roiUpper );
      candidatePeak->setMean( roiUpper );
    }
    
    candidatePeak->continuum()->setRange( roiLower, roiUpper );
    
    roiTypeChanged();
  };//roiRangeChanged
  
  
  
  energySB->changed().connect( std::bind(meanChanged) );
  fwhmSB->changed().connect( std::bind(fwhmChanged) );
  areaSB->changed().connect( std::bind(ampChanged) );
  roiLowerSB->changed().connect( std::bind(roiRangeChanged) );
  roiUpperSB->changed().connect( std::bind(roiRangeChanged) );
  contType->changed().connect( std::bind(roiTypeChanged) );
  
  WPushButton *closeButton = window->addCloseButtonToFooter("Cancel");
  closeButton->clicked().connect( window, &AuxWindow::hide );
  WPushButton *doAdd = new WPushButton( "Add" , window->footer() );
  
  doAdd->clicked().connect( std::bind( [=](){
    m_viewer->addPeak( *candidatePeak, false );
    window->hide();
  }) );
  
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );

  roiTypeChanged();
  updateCandidatePeakPreview();
  
  window->show();
  window->centerWindow();
  
  
  /*
  double width = 5.0;
  double amplitude = m_spectrumDisplayDiv->yAxisMaximum();

  if( m_model->npeaks() )
  {
    const PeakDef &firstPeak = m_model->peak( 0 );
    amplitude = firstPeak.peakArea();
    width = firstPeak.gausPeak() ? firstPeak.sigma() : 0.25*firstPeak.roiWidth();
  }//if( m_model->npeaks() )

  if( amplitude <= 0.0 )
    amplitude = 1000.0;

  
  std::shared_ptr<const Measurement> data = m_spectrumDisplayDiv->data();
  if( !data )
  {
    passMessage( "A foreground spectrum must be loaded to add a peak.",
                 "", WarningWidget::WarningMsgHigh );
    return;
  }
  
  const size_t nbin = data->num_gamma_channels();
  xmin = std::max( xmin, static_cast<double>(data->gamma_channel_lower(0)) );
  xmax = std::min( xmax, static_cast<double>(data->gamma_channel_upper(nbin-1)) );

  try
  {
    const double energy = ceil( xmin + 0.01*(xmax - xmin) );
    m_viewer->addPeak( PeakDef( energy, width, amplitude ), false );

    passMessage( "Newly created peak assigned initial mean of "
                 + std::to_string(energy)
                 + " keV; please edit peak.",
                "", WarningWidget::WarningMsgInfo );
  }catch( std::exception &e )
  {
    passMessage( "Error adding peak", "", WarningWidget::WarningMsgHigh );
    return;
  }
   */
}//void createNewPeak()


void PeakInfoDisplay::deleteSelectedPeak()
{
  //Basically, for lack of better way, we have to figure out which row is
  //  currently being edited, and then remove the peak corresponding to that
  //  row.
  //  Since we will probably have less than ~20 rows, I'll not bother to find
  //  a better method...
  const int nrow = m_model->rowCount();
  const int ncol = m_model->columnCount();

  m_infoView->closeEditors();
    
  for( int row = 0; row < nrow; ++row )
  {
    for( int col = 0; col < ncol; ++col )
    {
      const WModelIndex index = m_model->index( row, col );
      if( m_infoView->isEditing( index ) || m_infoView->isSelected(index))
      {
        m_model->removeRows( row, 1 );
        disablePeakDelete();
        return;
      }//if( we found the field being edditied
    }//for( loop over columns )
  }//for( loop over rows )
}//void deleteSelectedPeak();


void PeakInfoDisplay::enablePeakDelete( WModelIndex index )
{
  if( m_deletePeak->isEnabled() || !index.isValid() )
    return;

  const WFlags<ItemFlag> flags = m_model->flags( index );
  if( (flags & Wt::ItemIsSelectable) == Wt::ItemIsSelectable )
  {
    m_deletePeak->enable();
    m_deletePeak->show();
  }
}//void enablePeakDelete()


void PeakInfoDisplay::disablePeakDelete()
{
  //Check if any row is selected.  If none are selected, then disable this button.
  const int nrow = m_model->rowCount();
  const int ncol = m_model->columnCount();
    
  for( int row = 0; row < nrow; ++row )
  {
    for( int col = 0; col < ncol; ++col )
    {
      const WModelIndex index = m_model->index( row, col );
      if(m_infoView->isSelected(index))
      {
        m_deletePeak->enable();
        m_deletePeak->show();
        return;
      }//if( we found a row selected
    }//for( loop over columns )
  }//for( loop over rows )
    
  m_deletePeak->disable(); //default
  if( dynamic_cast<WImage *>(m_deletePeak) )
    m_deletePeak->hide();
}//void disablePeakDelete()


void PeakInfoDisplay::init()
{
  if( !m_model )
    throw runtime_error( "PeakInfoDisplay must be passed a valid PeakModel" );
  
  assert( !m_infoView );
  assert( !m_infoLayout );

  const bool showToolTipInstantly = InterSpecUser::preferenceValue<bool>( "ShowTooltips", m_viewer );
  
  m_infoLayout = new WGridLayout();
  setLayout( m_infoLayout );
  
  m_infoLayout->setContentsMargins( 0, 0, 0, 0 );
  m_infoLayout->setVerticalSpacing( 0 );
  m_infoLayout->setHorizontalSpacing( 0 );
  
  m_infoView = new RowStretchTreeView();
  m_infoView->addStyleClass( "PeakInfoDisplayTable" );
  m_infoView->setRootIsDecorated(	false); //makes the tree look like a table! :)
  
  m_infoView->setModel( m_model );

//  m_infoView->setRowHeight(28);
//  m_infoView->setHeaderHeight(28);
  m_infoView->setSortingEnabled( true );
  m_infoView->setAlternatingRowColors( true );
  m_infoView->setSelectable( true );
  m_infoView->setSelectionMode( SingleSelection );
  
  m_infoView->setColumnHidden( PeakModel::kCandidateIsotopes, true );
  m_infoView->setColumnHidden( PeakModel::kUseForCalibration, true );
  m_infoView->setColumnHidden( PeakModel::kUseForShieldingSourceFit, true );

  m_infoView->setEditTriggers( WAbstractItemView::SingleClicked | WAbstractItemView::DoubleClicked );

  WItemDelegate *dblDelagate = new WItemDelegate( m_infoView );
  dblDelagate->setTextFormat( "%.2f" );
  m_infoView->setItemDelegateForColumn( PeakModel::kMean, dblDelagate );
  m_infoView->setItemDelegateForColumn( PeakModel::kFwhm, dblDelagate );
  m_infoView->setItemDelegateForColumn( PeakModel::kLowerX, dblDelagate );
  m_infoView->setItemDelegateForColumn( PeakModel::kUpperX, dblDelagate );

  dblDelagate = new WItemDelegate( m_infoView );
  dblDelagate->setTextFormat( "%.0f" );
  m_infoView->setItemDelegateForColumn( PeakModel::kAmplitude, dblDelagate );
  m_infoView->setItemDelegateForColumn( PeakModel::kContinuumArea, dblDelagate );

  //tweak column widths
  m_infoView->setColumnWidth( PeakModel::kMean,       WLength(8, WLength::FontEx) );
  m_infoView->setColumnWidth( PeakModel::kFwhm,       WLength(9, WLength::FontEx) );
  m_infoView->setColumnWidth( PeakModel::kAmplitude,  WLength(12, WLength::FontEx) );

  m_infoView->setColumnWidth( PeakModel::kIsotope,    WLength(/*8*/9, WLength::FontEx) );
  //Note 20131211, wcjohns: closeOnBlur was previoulsy set to false, however
  //  there was a rare crash that happened with the model row was deleted while
  //  the editor was still open.  Hopefully closeOnBlur==true will fix this
  //  (appears to).
  const bool closeOnBlur = true;
  PhotopeakDelegate *nuclideDelegate = new PhotopeakDelegate( PhotopeakDelegate::NuclideDelegate, closeOnBlur, m_infoView );
  m_infoView->setItemDelegateForColumn( PeakModel::kIsotope, nuclideDelegate );

  m_infoView->setColumnWidth( PeakModel::kPhotoPeakEnergy, WLength(12 /*18*/, WLength::FontEx) );
  PhotopeakDelegate *photopeakDelegate = new PhotopeakDelegate( PhotopeakDelegate::GammaEnergyDelegate, closeOnBlur, m_infoView );
  m_infoView->setItemDelegateForColumn( PeakModel::kPhotoPeakEnergy, photopeakDelegate );

  m_infoView->setColumnWidth( PeakModel::kUserLabel,  WLength(9, WLength::FontEx) );
  
#if( ALLOW_PEAK_COLOR_DELEGATE )
  m_infoView->setColumnWidth( PeakModel::kPeakLineColor, WLength(6,WLength::FontEx) );
  ColorDelegate *colorDelegate = new ColorDelegate( m_infoView );
  m_infoView->setItemDelegateForColumn( PeakModel::kPeakLineColor, colorDelegate );
#else
  m_infoView->setColumnHidden( PeakModel::kPeakLineColor, true );
#endif
  
  m_infoView->setColumnHidden( PeakModel::kHasSkew, true );
//  m_infoView->setColumnWidth( PeakModel::kHasSkew,    WLength(9, WLength::FontEx) );
  
  m_infoView->setColumnHidden( PeakModel::kSkewAmount, true );
//  m_infoView->setColumnWidth( PeakModel::kSkewAmount, WLength(/*10*/13, WLength::FontEx) );
  
  m_infoView->setColumnHidden( PeakModel::kType, true );
//  m_infoView->setColumnWidth( PeakModel::kType,       WLength(12, WLength::FontEx) );
  
  m_infoView->setColumnWidth( PeakModel::kLowerX,     WLength(/*11*/16, WLength::FontEx) );
  m_infoView->setColumnWidth( PeakModel::kUpperX,     WLength(/*11*/16, WLength::FontEx) );
  m_infoView->setColumnWidth( PeakModel::kContinuumArea, WLength(/*13*/16, WLength::FontEx) );
  m_infoView->setColumnWidth( PeakModel::kContinuumType,  WLength(/*11*/16, WLength::FontEx) );
//  m_infoView->setColumnWidth( PeakModel::kNumColumns, WLength(6, WLength::FontEx) );
//  m_infoView->setColumnAlignment( PeakModel::kMean, AlignRight );

//  Selection mode seems to have no effect....
//  m_infoView->setSelectionBehavior( Wt::SelectRows );
//  m_infoView->setSelectionMode( Wt::SingleSelection );
  m_infoView->setEditOptions( WAbstractItemView::SingleEditor );

  m_infoView->clicked().connect( boost::bind( &PeakInfoDisplay::enablePeakDelete, this, _1 ) );
  m_infoView->doubleClicked().connect( boost::bind( &PeakInfoDisplay::enablePeakDelete, this, _1 ) );

  m_infoLayout->addWidget( m_infoView, 0, 0 );
  m_infoLayout->setRowStretch( 0, 1 );
  

  //Now add buttons to search/clear peaks
  WContainerWidget *buttonDiv = new WContainerWidget();
  buttonDiv->addStyleClass( "PeakInfoDisplayButtonDiv" );
  m_infoLayout->addWidget( buttonDiv, 1, 0 );

  m_searchForPeaks = new WPushButton( "Search for Peaks", buttonDiv );
  m_searchForPeaks->addStyleClass("FindIcon");
  m_searchForPeaks->setIcon( "InterSpec_resources/images/magnifier.png" );
  
  //m_searchForPeaks->setMargin(WLength(7),Wt::Left);
  //m_searchForPeaks->setMargin(WLength(3),Wt::Bottom);
  HelpSystem::attachToolTipOn( m_searchForPeaks, "Search for peaks using the automated peak finding "
                              "algorithm.", showToolTipInstantly, HelpSystem::Top  );
  m_searchForPeaks->clicked().connect( boost::bind( &PeakSearchGuiUtils::automated_search_for_peaks, m_viewer, true ) );

  
  WPushButton *clearPeaksButton = new WPushButton( "Clear all Peaks", buttonDiv );
  HelpSystem::attachToolTipOn( clearPeaksButton, "Removes <b>all</b> existing peaks.", showToolTipInstantly, HelpSystem::Top  );
  
  //clearPeaksButton->setMargin(WLength(2),Wt::Left);
  clearPeaksButton->clicked().connect( this, &PeakInfoDisplay::confirmRemoveAllPeaks );
  clearPeaksButton->disable();

  //"Nuc. from Ref."
  WPushButton *nucFromRefButton = new WPushButton( "Nuc. from Ref.", buttonDiv );
  nucFromRefButton->setIcon( "InterSpec_resources/images/assign_white.png" );
  
  //button->setMargin(WLength(2),Wt::Left|Wt::Right);
  HelpSystem::attachToolTipOn( nucFromRefButton,
                              "Assign peak nuclides from reference lines showing. Only applies to "
                              "peaks which do not already have a nuclide associated "
                              "with them." ,
                              showToolTipInstantly , HelpSystem::Top );
  nucFromRefButton->clicked().connect( boost::bind( &PeakInfoDisplay::assignNuclidesFromRefLines, this ) );
  nucFromRefButton->disable();
  
  auto enableDisableNucRef = [this,nucFromRefButton,clearPeaksButton](){
    const bool enable = (m_model->rowCount() > 0);
    clearPeaksButton->setEnabled( enable );
    nucFromRefButton->setEnabled( enable );
    //Should check if any reference lines are showing for nucFromRefButton as well...
  };
  
  m_model->dataChanged().connect( std::bind(enableDisableNucRef) );
  m_model->rowsRemoved().connect( std::bind(enableDisableNucRef) );
  m_model->rowsInserted().connect( std::bind(enableDisableNucRef) );
  m_model->layoutChanged().connect( std::bind(enableDisableNucRef) );
  
/*
  WPushButton *button = new WPushButton( "ID Nuclides", buttonDiv );
  button->setIcon( "InterSpec_resources/images/assign_white.png" );
  button->setMargin(WLength(2),Wt::Left|Wt::Right);
  HelpSystem::attachToolTipOn( button,
                              "Guess nuclides responsible for peaks. Only applies to "
                              "peaks which do not already have a nuclide associated "
                              "with them.  Works best once all peaks have been fit for." ,
                              showToolTipInstantly , HelpSystem::Top );
  button->clicked().connect( boost::bind( &InterSpec::guessIsotopesForPeaks, m_viewer, (WApplication *)0 ) );
*/
  
  //If you want to post the below, so the ID isnt carried out in the main event loop, use the following
//  boost::function<void ()> guessIsotopeWorker = boost::bind( &InterSpec::guessIsotopesForPeaks, m_viewer, wApp );
//  button->clicked().connect( boost::bind( &WServer::post, WServer::instance(),
//                                           wApp->sessionId(), guessIsotopeWorker,
//                                           boost::function<void ()>() ) );
  
  WLabel *label = new WLabel("Peak: ", buttonDiv);
  label->addStyleClass("buttonSeparator");
  label->setMargin(WLength(10),Wt::Left);
  
  if( m_viewer->isMobile() )
  {
    WImage *addPeak = new WImage( WLink("InterSpec_resources/images/plus_min_black.svg"), buttonDiv );
    addPeak->setAttributeValue( "width", "16" );
    addPeak->setAttributeValue( "height", "16" );
    addPeak->addStyleClass( "WhiteIcon" );
    addPeak->setMargin( 2, Wt::Left );
    addPeak->setMargin( 8, Wt::Right );
    addPeak->setMargin( 5, Wt::Top );
    addPeak->clicked().connect( this, &PeakInfoDisplay::createNewPeak );
    
    WImage *delPeak = new WImage( WLink("InterSpec_resources/images/minus_min_black.svg"), buttonDiv );
    delPeak->setAttributeValue( "width", "16" );
    delPeak->setAttributeValue( "height", "16" );
    delPeak->addStyleClass( "WhiteIcon" );
    delPeak->setMargin( 5, Wt::Top );
    delPeak->clicked().connect( this, &PeakInfoDisplay::deleteSelectedPeak );
    m_deletePeak = delPeak;
    m_deletePeak->hide();
  }else
  {
    WPushButton *addPeak = new WPushButton( "Add...", buttonDiv );
    HelpSystem::attachToolTipOn( addPeak, "Manually add a new peak.", showToolTipInstantly, HelpSystem::Top );
    addPeak->clicked().connect( this, &PeakInfoDisplay::createNewPeak );
    addPeak->setIcon( "InterSpec_resources/images/plus_min_white.png" );
    
    WPushButton *delPeak = new WPushButton( "Delete", buttonDiv );
    HelpSystem::attachToolTipOn( delPeak, "Deletes peak currently being edited.", showToolTipInstantly, HelpSystem::Top  );
    delPeak->setHiddenKeepsGeometry( true );
    delPeak->clicked().connect( this, &PeakInfoDisplay::deleteSelectedPeak );
    delPeak->setIcon( "InterSpec_resources/images/minus_min_white.png" );
    m_deletePeak = delPeak;
  }//if( mobile ) / else
  
  m_deletePeak->disable();
  
  
  //Whenver a delegate gets closed, lets disable the peak delete button
  set<WAbstractItemDelegate *> uniqueDelegates;
  for( int col = 0; col < m_model->columnCount(); ++col )
  {
    WAbstractItemDelegate *delegate = m_infoView->itemDelegateForColumn( col );
    if( delegate && !uniqueDelegates.count(delegate) )
    {
      delegate->closeEditor().connect( this, &PeakInfoDisplay::disablePeakDelete );
      uniqueDelegates.insert( delegate );
    }
  }//for( int col = 0; col < m_model->columnCount(); ++col )

#if( !ANDROID && !IOS )
  WText *txt = new WText( "Right click on peaks for better editor", buttonDiv );
  txt->addStyleClass( "PeakEditHint" );
  txt->hide();
//  mouseWentOver().connect( txt, &WText::show );
//  mouseWentOut().connect( txt, &WText::hide );
//  doJavaScript( "try{$('#" + txt->id() + "').show();}catch(e){}" );
  mouseWentOver().connect( "function(object, event){try{$('#" + txt->id() + "').show();}catch(e){}}" );
  mouseWentOut().connect( "function(object, event){try{$('#" + txt->id() + "').hide();}catch(e){}}" );
  
  
  WResource *csv = m_model->peakCsvResource();
#if( BUILD_AS_OSX_APP )
  WAnchor *csvButton = new WAnchor( WLink(csv), buttonDiv );
  csvButton->setTarget( AnchorTarget::TargetNewWindow );
#else
  WPushButton *csvButton = new WPushButton( buttonDiv );
  csvButton->setIcon( "InterSpec_resources/images/download_small.png" );
  csvButton->setLink( WLink(csv) );
  csvButton->setLinkTarget( Wt::TargetNewWindow );
#endif
  
  csvButton->setText( "CSV" );
  csvButton->setStyleClass( "CsvLinkBtn" );
  csvButton->disable();

  auto enableDisableCsv = [this,csvButton](){
    //csvButton->setEnabled( m_model->rowCount() > 0 );
    if( m_model->rowCount() > 0 )
      csvButton->enable();
    else
      csvButton->disable();
  };
  
  m_model->dataChanged().connect( std::bind(enableDisableCsv) );
  m_model->rowsRemoved().connect( std::bind(enableDisableCsv) );
  m_model->rowsInserted().connect( std::bind(enableDisableCsv) );
  m_model->layoutChanged().connect( std::bind(enableDisableCsv) );
  
  HelpSystem::attachToolTipOn( csvButton,"Export information about the identified peaks to a "
                              "comma seperated format.", showToolTipInstantly );
#endif
  
}//init()


