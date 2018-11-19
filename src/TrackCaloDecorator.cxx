#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"

// tracks
#include "TrkTrack/Track.h"
#include "TrkParameters/TrackParameters.h"
#include "TrkExInterfaces/IExtrapolator.h"
#include "xAODTracking/TrackParticleContainer.h"

// extrapolation to the calo
#include "TrkCaloExtension/CaloExtension.h"
#include "TrkCaloExtension/CaloExtensionCollection.h"
#include "TrkParametersIdentificationHelpers/TrackParametersIdHelper.h"
#include "CaloDetDescr/CaloDepthTool.h"

// calo and cell information
#include "TileEvent/TileContainer.h"
#include "TileIdentifier/TileTBID.h"
#include "CaloEvent/CaloCellContainer.h"
#include "CaloTrackingGeometry/ICaloSurfaceHelper.h"
#include "TrkSurfaces/DiscSurface.h"
#include "GeoPrimitives/GeoPrimitives.h"
#include "CaloEvent/CaloClusterContainer.h"
#include "CaloEvent/CaloCluster.h"
#include "CaloUtils/CaloClusterSignalState.h"
#include "xAODEventInfo/EventInfo.h"
#include "CaloEvent/CaloClusterCellLinkContainer.h"
#include "xAODCaloEvent/CaloClusterChangeSignalState.h"

#include <utility>
#include <vector>
#include <cmath>
#include <limits>

namespace DerivationFramework {

  TrackCaloDecorator::TrackCaloDecorator(const std::string& t, const std::string& n, const IInterface* p) : 
    AthAlgTool(t,n,p), //type, name, parent
    m_sgName(""),
    m_eventInfoContainerName("EventInfo"),
    m_trackContainerName("InDetTrackParticles"),
    m_caloClusterContainerName("CaloCalTopoClusters"),
    m_extrapolator("Trk::Extrapolator"),
    m_theTrackExtrapolatorTool("Trk::ParticleCaloExtensionTool"),
    m_trackParametersIdHelper(new Trk::TrackParametersIdHelper),
    m_surfaceHelper("CaloSurfaceHelper/CaloSurfaceHelper"),
    m_tileTBID(0),
    m_doCutflow{false}{
      declareInterface<DerivationFramework::IAugmentationTool>(this);
      declareProperty("DecorationPrefix", m_sgName);
      declareProperty("EventContainer", m_eventInfoContainerName);
      declareProperty("TrackContainer", m_trackContainerName);
      declareProperty("CaloClusterContainer", m_caloClusterContainerName);
      declareProperty("Extrapolator", m_extrapolator);
      declareProperty("TheTrackExtrapolatorTool", m_theTrackExtrapolatorTool);
      declareProperty("DoCutflow", m_doCutflow);
    }

  StatusCode TrackCaloDecorator::initialize()
  {
    if (m_sgName=="") {
      ATH_MSG_WARNING("No decoration prefix name provided for the output of TrackCaloDecorator!");
    }

    ATH_CHECK(m_extrapolator.retrieve());

    ATH_CHECK(m_theTrackExtrapolatorTool.retrieve());

    ATH_CHECK(m_surfaceHelper.retrieve());

    // Get the test beam identifier for the MBTS
    ATH_CHECK(detStore()->retrieve(m_tileTBID));

    // Save cutflow histograms
    if (m_doCutflow) {
      ServiceHandle<ITHistSvc> histSvc("THistSvc", name()); 
      CHECK( histSvc.retrieve() );

      m_cutflow_evt = new TH1F("cutflow_evt", "cutflow_evt", 1, 1, 2);
      m_cutflow_evt->SetCanExtend(TH1::kAllAxes);
      m_cutflow_evt_all = m_cutflow_evt->GetXaxis()->FindBin("Total events");
      m_cutflow_evt_pass_all = m_cutflow_evt->GetXaxis()->FindBin("Pass all");
      histSvc->regHist("/CutflowStream/cutflow_evt", m_cutflow_evt).ignore(); 

      m_cutflow_trk = new TH1F("cutflow_trk", "cutflow_trk", 1, 1, 2);
      m_cutflow_trk->SetCanExtend(TH1::kAllAxes);
      m_cutflow_trk_all = m_cutflow_trk->GetXaxis()->FindBin("Total tracks");
      m_cutflow_trk_pass_extrapolation = m_cutflow_trk->GetXaxis()->FindBin("Pass extrapolation");
      m_cutflow_trk_pass_cluster_matching = m_cutflow_trk->GetXaxis()->FindBin("Pass trk-cluster matching");
      m_cutflow_trk_pass_cell_matching = m_cutflow_trk->GetXaxis()->FindBin("Pass trk-cell matching");
      m_cutflow_trk_pass_loop_matched_clusters = m_cutflow_trk->GetXaxis()->FindBin("Pass loop matched clusters");
      m_cutflow_trk_pass_loop_matched_cells = m_cutflow_trk->GetXaxis()->FindBin("Pass loop matched cells");
      m_cutflow_trk_pass_all = m_cutflow_trk->GetXaxis()->FindBin("Pass all");
      histSvc->regHist("/CutflowStream/cutflow_trk", m_cutflow_trk).ignore(); 

      m_ntrks_per_event_all = new TH1F("ntrks_per_event_all", "ntrks_per_event_all", 300, 0, 300);
      histSvc->regHist("/CutflowStream/ntrks_per_event_all", m_ntrks_per_event_all).ignore(); 

      m_ntrks_per_event_pass_all = new TH1F("ntrks_per_event_pass_all", "ntrks_per_event_pass_all", 300, 0, 300);
      histSvc->regHist("/CutflowStream/ntrks_per_event_pass_all", m_ntrks_per_event_pass_all).ignore(); 

      m_tree = new TTree("cutflow_tree","cutflow_tree");
      CHECK( histSvc->regTree("/CutflowStream/cutflow_tree", m_tree) );

      m_tree->Branch("runNumber", &m_runNumber, "runNumber/I");
      m_tree->Branch("eventNumber", &m_eventNumber, "eventNumber/I");
      m_tree->Branch("lumiBlock", &m_lumiBlock, "lumiBlock/I");
      m_tree->Branch("nTrks", &m_nTrks, "nTrks/I");
      m_tree->Branch("nTrks_pass", &m_nTrks_pass, "nTrks_pass/I");
    }

    return StatusCode::SUCCESS;
  }

  StatusCode TrackCaloDecorator::finalize() {
    return StatusCode::SUCCESS;
  }

  StatusCode TrackCaloDecorator::addBranches() const {

    /*---------E/p Decorators---------*/
    /*---------Calo Sample layer Variables---------*/
    // PreSamplerB=0, EMB1, EMB2, EMB3,       // LAr barrel
    // PreSamplerE, EME1, EME2, EME3,         // LAr EM endcap
    // HEC0, HEC1, HEC2, HEC3,                // Hadronic end cap cal.
    // TileBar0, TileBar1, TileBar2,          // Tile barrel
    // TileGap1, TileGap2, TileGap3,          // Tile gap (ITC & scint)
    // TileExt0, TileExt1, TileExt2,          // Tile extended barrel
    // FCAL0, FCAL1, FCAL2,                   // Forward EM endcap (excluded)
    // Unknown

    //Building decorators this way instead of using auxdecor<> saves time//  
    static SG::AuxElement::Decorator<int> decorator_extrapolation (m_sgName + "_extrapolation");

    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Energy (m_sgName + "_ClusterEnergy_Energy");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Eta (m_sgName + "_ClusterEnergy_Eta");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Phi (m_sgName + "_ClusterEnergy_Phi");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_dRToTrack (m_sgName + "_ClusterEnergy_dRToTrack");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_lambdaCenter (m_sgName + "_ClusterEnergy_lambdaCenter");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_emProbability (m_sgName + "_ClusterEnergy_emProbability");
    static SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergy_maxEnergyLayer (m_sgName + "_ClusterEnergy_maxEnergyLayer");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_firstEnergyDensity (m_sgName + "_ClusterEnergy_firstEnergyDensity");

    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Energy (m_sgName + "_ClusterEnergyLCW_Energy");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Eta (m_sgName + "_ClusterEnergyLCW_Eta");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Phi (m_sgName + "_ClusterEnergyLCW_Phi");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_dRToTrack (m_sgName + "_ClusterEnergyLCW_dRToTrack");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_lambdaCenter (m_sgName + "_ClusterEnergyLCW_lambdaCenter");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_emProbability (m_sgName + "_ClusterEnergyLCW_emProbability");
    static SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergyLCW_maxEnergyLayer (m_sgName + "_ClusterEnergyLCW_maxEnergyLayer");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_firstEnergyDensity (m_sgName + "_ClusterEnergyLCW_firstEnergyDensity");

    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_Energy (m_sgName + "_CellEnergy_Energy");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_Eta (m_sgName + "_CellEnergy_Eta");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_Phi (m_sgName + "_CellEnergy_Phi");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_dRToTrack (m_sgName + "_CellEnergy_dRToTrack");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_lambdaCenter (m_sgName + "_CellEnergy_lambdaCenter");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_emProbability (m_sgName + "_CellEnergy_emProbability");
    static SG::AuxElement::Decorator< std::vector<int> > decorator_CellEnergy_maxEnergyLayer (m_sgName + "_CellEnergy_maxEnergyLayer");
    static SG::AuxElement::Decorator< std::vector<float> > decorator_CellEnergy_firstEnergyDensity (m_sgName + "_CellEnergy_firstEnergyDensity");

    ///////////////Presamplers///////////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_PreSamplerB (m_sgName + "_trkEta_PreSamplerB");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_PreSamplerB (m_sgName + "_trkPhi_PreSamplerB");
    static SG::AuxElement::Decorator<float> decorator_trkEta_PreSamplerE (m_sgName + "_trkEta_PreSamplerE");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_PreSamplerE (m_sgName + "_trkPhi_PreSamplerE");

    /*---------EM LAYER Variables---------*/
    ///////////LAr EM Barrel layers///////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_EMB1 (m_sgName + "_trkEta_EMB1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EMB1 (m_sgName + "_trkPhi_EMB1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_EMB2 (m_sgName + "_trkEta_EMB2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EMB2 (m_sgName + "_trkPhi_EMB2");
    static SG::AuxElement::Decorator<float> decorator_trkEta_EMB3 (m_sgName + "_trkEta_EMB3");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EMB3 (m_sgName + "_trkPhi_EMB3");
    ///////////LAr EM Endcap layers///////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_EME1 (m_sgName + "_trkEta_EME1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EME1 (m_sgName + "_trkPhi_EME1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_EME2 (m_sgName + "_trkEta_EME2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EME2 (m_sgName + "_trkPhi_EME2");
    static SG::AuxElement::Decorator<float> decorator_trkEta_EME3 (m_sgName + "_trkEta_EME3");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_EME3 (m_sgName + "_trkPhi_EME3");

    /*---------HAD LAYER Variables---------*/
    ///////////Hadronic Endcap layers/////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_HEC0 (m_sgName + "_trkEta_HEC0");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_HEC0 (m_sgName + "_trkPhi_HEC0");
    static SG::AuxElement::Decorator<float> decorator_trkEta_HEC1 (m_sgName + "_trkEta_HEC1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_HEC1 (m_sgName + "_trkPhi_HEC1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_HEC2 (m_sgName + "_trkEta_HEC2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_HEC2 (m_sgName + "_trkPhi_HEC2");
    static SG::AuxElement::Decorator<float> decorator_trkEta_HEC3 (m_sgName + "_trkEta_HEC3");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_HEC3 (m_sgName + "_trkPhi_HEC3");
    ///////////Tile Barrel layers///////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileBar0 (m_sgName + "_trkEta_TileBar0");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileBar0 (m_sgName + "_trkPhi_TileBar0");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileBar1 (m_sgName + "_trkEta_TileBar1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileBar1 (m_sgName + "_trkPhi_TileBar1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileBar2 (m_sgName + "_trkEta_TileBar2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileBar2 (m_sgName + "_trkPhi_TileBar2");
    ////////////Tile Gap layers////////////
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileGap1 (m_sgName + "_trkEta_TileGap1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileGap1 (m_sgName + "_trkPhi_TileGap1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileGap2 (m_sgName + "_trkEta_TileGap2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileGap2 (m_sgName + "_trkPhi_TileGap2");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileGap3 (m_sgName + "_trkEta_TileGap3");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileGap3 (m_sgName + "_trkPhi_TileGap3");
    //////Tile Extended Barrel layers//////
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileExt0 (m_sgName + "_trkEta_TileExt0");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileExt0 (m_sgName + "_trkPhi_TileExt0");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileExt1 (m_sgName + "_trkEta_TileExt1");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileExt1 (m_sgName + "_trkPhi_TileExt1");
    static SG::AuxElement::Decorator<float> decorator_trkEta_TileExt2 (m_sgName + "_trkEta_TileExt2");
    static SG::AuxElement::Decorator<float> decorator_trkPhi_TileExt2 (m_sgName + "_trkPhi_TileExt2");

    /*FCAL and MINIFCAL layers excluded as tracking acceptance does not extend to these*/

    /*EM scale cluster energy within a cone of DeltraR = 0.2 (200) / 0.1 (100) around the extrapolated tracks*/
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_PreSamplerB_200 (m_sgName + "_ClusterEnergy_PreSamplerB_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_PreSamplerB_100 (m_sgName + "_ClusterEnergy_PreSamplerB_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_PreSamplerE_200 (m_sgName + "_ClusterEnergy_PreSamplerE_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_PreSamplerE_100 (m_sgName + "_ClusterEnergy_PreSamplerE_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB1_200 (m_sgName + "_ClusterEnergy_EMB1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB1_100 (m_sgName + "_ClusterEnergy_EMB1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB2_200 (m_sgName + "_ClusterEnergy_EMB2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB2_100 (m_sgName + "_ClusterEnergy_EMB2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB3_200 (m_sgName + "_ClusterEnergy_EMB3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EMB3_100 (m_sgName + "_ClusterEnergy_EMB3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME1_200 (m_sgName + "_ClusterEnergy_EME1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME1_100 (m_sgName + "_ClusterEnergy_EME1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME2_200 (m_sgName + "_ClusterEnergy_EME2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME2_100 (m_sgName + "_ClusterEnergy_EME2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME3_200 (m_sgName + "_ClusterEnergy_EME3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_EME3_100 (m_sgName + "_ClusterEnergy_EME3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC0_200 (m_sgName + "_ClusterEnergy_HEC0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC0_100 (m_sgName + "_ClusterEnergy_HEC0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC1_200 (m_sgName + "_ClusterEnergy_HEC1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC1_100 (m_sgName + "_ClusterEnergy_HEC1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC2_200 (m_sgName + "_ClusterEnergy_HEC2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC2_100 (m_sgName + "_ClusterEnergy_HEC2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC3_200 (m_sgName + "_ClusterEnergy_HEC3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_HEC3_100 (m_sgName + "_ClusterEnergy_HEC3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar0_200 (m_sgName + "_ClusterEnergy_TileBar0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar0_100 (m_sgName + "_ClusterEnergy_TileBar0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar1_200 (m_sgName + "_ClusterEnergy_TileBar1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar1_100 (m_sgName + "_ClusterEnergy_TileBar1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar2_200 (m_sgName + "_ClusterEnergy_TileBar2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileBar2_100 (m_sgName + "_ClusterEnergy_TileBar2_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap1_200 (m_sgName + "_ClusterEnergy_TileGap1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap1_100 (m_sgName + "_ClusterEnergy_TileGap1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap2_200 (m_sgName + "_ClusterEnergy_TileGap2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap2_100 (m_sgName + "_ClusterEnergy_TileGap2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap3_200 (m_sgName + "_ClusterEnergy_TileGap3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileGap3_100 (m_sgName + "_ClusterEnergy_TileGap3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt0_200 (m_sgName + "_ClusterEnergy_TileExt0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt0_100 (m_sgName + "_ClusterEnergy_TileExt0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt1_200 (m_sgName + "_ClusterEnergy_TileExt1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt1_100 (m_sgName + "_ClusterEnergy_TileExt1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt2_200 (m_sgName + "_ClusterEnergy_TileExt2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergy_TileExt2_100 (m_sgName + "_ClusterEnergy_TileExt2_100");

    /*Total EM scale cluster energy for the EM and/or HAD calorimeters*/
    static SG::AuxElement::Decorator<float> decorator_EM_ClusterEnergy_0_200 (m_sgName + "_EM_ClusterEnergy_0_200"); //Capital lettering is to match Run 1 designations
    static SG::AuxElement::Decorator<float> decorator_HAD_ClusterEnergy_0_200 (m_sgName + "_HAD_ClusterEnergy_0_200");
    static SG::AuxElement::Decorator<float> decorator_EM_ClusterEnergy_0_100 (m_sgName + "_EM_ClusterEnergy_0_100");
    static SG::AuxElement::Decorator<float> decorator_HAD_ClusterEnergy_0_100 (m_sgName + "_HAD_ClusterEnergy_0_100");
    static SG::AuxElement::Decorator<float> decorator_Total_ClusterEnergy_0_200 (m_sgName + "_Total_ClusterEnergy_0_200");
    static SG::AuxElement::Decorator<float> decorator_Total_ClusterEnergy_0_100 (m_sgName + "_Total_ClusterEnergy_0_100");

    /*LCW scale cluster energy within a cone of DeltraR = 0.2 (200) / 0.1 (100) around the extrapolated tracks*/
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_PreSamplerB_200 (m_sgName + "_ClusterEnergyLCW_PreSamplerB_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_PreSamplerB_100 (m_sgName + "_ClusterEnergyLCW_PreSamplerB_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_PreSamplerE_200 (m_sgName + "_ClusterEnergyLCW_PreSamplerE_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_PreSamplerE_100 (m_sgName + "_ClusterEnergyLCW_PreSamplerE_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB1_200 (m_sgName + "_ClusterEnergyLCW_EMB1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB1_100 (m_sgName + "_ClusterEnergyLCW_EMB1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB2_200 (m_sgName + "_ClusterEnergyLCW_EMB2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB2_100 (m_sgName + "_ClusterEnergyLCW_EMB2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB3_200 (m_sgName + "_ClusterEnergyLCW_EMB3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EMB3_100 (m_sgName + "_ClusterEnergyLCW_EMB3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME1_200 (m_sgName + "_ClusterEnergyLCW_EME1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME1_100 (m_sgName + "_ClusterEnergyLCW_EME1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME2_200 (m_sgName + "_ClusterEnergyLCW_EME2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME2_100 (m_sgName + "_ClusterEnergyLCW_EME2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME3_200 (m_sgName + "_ClusterEnergyLCW_EME3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_EME3_100 (m_sgName + "_ClusterEnergyLCW_EME3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC0_200 (m_sgName + "_ClusterEnergyLCW_HEC0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC0_100 (m_sgName + "_ClusterEnergyLCW_HEC0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC1_200 (m_sgName + "_ClusterEnergyLCW_HEC1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC1_100 (m_sgName + "_ClusterEnergyLCW_HEC1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC2_200 (m_sgName + "_ClusterEnergyLCW_HEC2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC2_100 (m_sgName + "_ClusterEnergyLCW_HEC2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC3_200 (m_sgName + "_ClusterEnergyLCW_HEC3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_HEC3_100 (m_sgName + "_ClusterEnergyLCW_HEC3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar0_200 (m_sgName + "_ClusterEnergyLCW_TileBar0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar0_100 (m_sgName + "_ClusterEnergyLCW_TileBar0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar1_200 (m_sgName + "_ClusterEnergyLCW_TileBar1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar1_100 (m_sgName + "_ClusterEnergyLCW_TileBar1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar2_200 (m_sgName + "_ClusterEnergyLCW_TileBar2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileBar2_100 (m_sgName + "_ClusterEnergyLCW_TileBar2_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap1_200 (m_sgName + "_ClusterEnergyLCW_TileGap1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap1_100 (m_sgName + "_ClusterEnergyLCW_TileGap1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap2_200 (m_sgName + "_ClusterEnergyLCW_TileGap2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap2_100 (m_sgName + "_ClusterEnergyLCW_TileGap2_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap3_200 (m_sgName + "_ClusterEnergyLCW_TileGap3_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileGap3_100 (m_sgName + "_ClusterEnergyLCW_TileGap3_100");

    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt0_200 (m_sgName + "_ClusterEnergyLCW_TileExt0_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt0_100 (m_sgName + "_ClusterEnergyLCW_TileExt0_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt1_200 (m_sgName + "_ClusterEnergyLCW_TileExt1_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt1_100 (m_sgName + "_ClusterEnergyLCW_TileExt1_100");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt2_200 (m_sgName + "_ClusterEnergyLCW_TileExt2_200");
    static SG::AuxElement::Decorator<float> decorator_ClusterEnergyLCW_TileExt2_100 (m_sgName + "_ClusterEnergyLCW_TileExt2_100");

    /*Total LCW scale cluster energy for the EM and/or HAD calorimeters*/
    static SG::AuxElement::Decorator<float> decorator_EM_ClusterEnergyLCW_0_200 (m_sgName + "_EM_ClusterEnergyLCW_0_200"); //Capital lettering is to match Run 1 designations
    static SG::AuxElement::Decorator<float> decorator_HAD_ClusterEnergyLCW_0_200 (m_sgName + "_HAD_ClusterEnergyLCW_0_200");
    static SG::AuxElement::Decorator<float> decorator_EM_ClusterEnergyLCW_0_100 (m_sgName + "_EM_ClusterEnergyLCW_0_100");
    static SG::AuxElement::Decorator<float> decorator_HAD_ClusterEnergyLCW_0_100 (m_sgName + "_HAD_ClusterEnergyLCW_0_100");
    static SG::AuxElement::Decorator<float> decorator_Total_ClusterEnergyLCW_0_200 (m_sgName + "_Total_ClusterEnergyLCW_0_200");
    static SG::AuxElement::Decorator<float> decorator_Total_ClusterEnergyLCW_0_100 (m_sgName + "_Total_ClusterEnergyLCW_0_100");

    /*Cell energy within a cone of DeltraR = 0.2 (200) / 0.1 (100) around the extrapolated tracks*/
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_PreSamplerB_200 (m_sgName + "_CellEnergy_PreSamplerB_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_PreSamplerB_100 (m_sgName + "_CellEnergy_PreSamplerB_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_PreSamplerE_200 (m_sgName + "_CellEnergy_PreSamplerE_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_PreSamplerE_100 (m_sgName + "_CellEnergy_PreSamplerE_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB1_200 (m_sgName + "_CellEnergy_EMB1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB1_100 (m_sgName + "_CellEnergy_EMB1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB2_200 (m_sgName + "_CellEnergy_EMB2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB2_100 (m_sgName + "_CellEnergy_EMB2_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB3_200 (m_sgName + "_CellEnergy_EMB3_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EMB3_100 (m_sgName + "_CellEnergy_EMB3_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME1_200 (m_sgName + "_CellEnergy_EME1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME1_100 (m_sgName + "_CellEnergy_EME1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME2_200 (m_sgName + "_CellEnergy_EME2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME2_100 (m_sgName + "_CellEnergy_EME2_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME3_200 (m_sgName + "_CellEnergy_EME3_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_EME3_100 (m_sgName + "_CellEnergy_EME3_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC0_200 (m_sgName + "_CellEnergy_HEC0_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC0_100 (m_sgName + "_CellEnergy_HEC0_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC1_200 (m_sgName + "_CellEnergy_HEC1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC1_100 (m_sgName + "_CellEnergy_HEC1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC2_200 (m_sgName + "_CellEnergy_HEC2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC2_100 (m_sgName + "_CellEnergy_HEC2_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC3_200 (m_sgName + "_CellEnergy_HEC3_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_HEC3_100 (m_sgName + "_CellEnergy_HEC3_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar0_200 (m_sgName + "_CellEnergy_TileBar0_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar0_100 (m_sgName + "_CellEnergy_TileBar0_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar1_200 (m_sgName + "_CellEnergy_TileBar1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar1_100 (m_sgName + "_CellEnergy_TileBar1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar2_200 (m_sgName + "_CellEnergy_TileBar2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileBar2_100 (m_sgName + "_CellEnergy_TileBar2_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap1_200 (m_sgName + "_CellEnergy_TileGap1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap1_100 (m_sgName + "_CellEnergy_TileGap1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap2_200 (m_sgName + "_CellEnergy_TileGap2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap2_100 (m_sgName + "_CellEnergy_TileGap2_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap3_200 (m_sgName + "_CellEnergy_TileGap3_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileGap3_100 (m_sgName + "_CellEnergy_TileGap3_100");

    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt0_200 (m_sgName + "_CellEnergy_TileExt0_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt0_100 (m_sgName + "_CellEnergy_TileExt0_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt1_200 (m_sgName + "_CellEnergy_TileExt1_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt1_100 (m_sgName + "_CellEnergy_TileExt1_100");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt2_200 (m_sgName + "_CellEnergy_TileExt2_200");
    static SG::AuxElement::Decorator<float> decorator_CellEnergy_TileExt2_100 (m_sgName + "_CellEnergy_TileExt2_100");

    /*Total cell energy for the EM and/or HAD calorimeters*/
    static SG::AuxElement::Decorator<float> decorator_EM_CellEnergy_0_200 (m_sgName + "_EM_CellEnergy_0_200");
    static SG::AuxElement::Decorator<float> decorator_HAD_CellEnergy_0_200 (m_sgName + "_HAD_CellEnergy_0_200");
    static SG::AuxElement::Decorator<float> decorator_EM_CellEnergy_0_100 (m_sgName + "_EM_CellEnergy_0_100");
    static SG::AuxElement::Decorator<float> decorator_HAD_CellEnergy_0_100 (m_sgName + "_HAD_CellEnergy_0_100");
    static SG::AuxElement::Decorator<float> decorator_Total_CellEnergy_0_200 (m_sgName + "_Total_CellEnergy_0_200");
    static SG::AuxElement::Decorator<float> decorator_Total_CellEnergy_0_100 (m_sgName + "_Total_CellEnergy_0_100");

    // Retrieve track container, Cluster container and Cell container
    const xAOD::TrackParticleContainer* trackContainer = 0;
    CHECK(evtStore()->retrieve(trackContainer, m_trackContainerName));

    const xAOD::CaloClusterContainer* clusterContainer = 0; //xAOD object used to create decorations.
    CHECK(evtStore()->retrieve(clusterContainer, m_caloClusterContainerName));

    const CaloCellContainer *caloCellContainer = 0; //ESD object used to create decorations
    CHECK(evtStore()->retrieve(caloCellContainer, "AllCalo"));

    bool evt_pass_all = false;
    int ntrks_all = 0;
    int ntrks_pass_all = 0;
    if (m_doCutflow) {
      m_cutflow_evt -> Fill(m_cutflow_evt_all, 1);
    }
    for (const auto& track : *trackContainer) {
      if (m_doCutflow) {
        ntrks_all++;
        m_cutflow_trk -> Fill(m_cutflow_trk_all, 1);
      }

      // Need to record a value for every track, so using -999999999 as an invalid code
      decorator_extrapolation (*track) = 0;

      decorator_ClusterEnergy_Energy (*track) = std::vector<float>();
      decorator_ClusterEnergy_Eta (*track) = std::vector<float>();
      decorator_ClusterEnergy_Phi (*track) = std::vector<float>();
      decorator_ClusterEnergy_dRToTrack (*track) = std::vector<float>();
      decorator_ClusterEnergy_emProbability (*track) = std::vector<float>();
      decorator_ClusterEnergy_firstEnergyDensity (*track) = std::vector<float>();
      decorator_ClusterEnergy_lambdaCenter (*track) = std::vector<float>();
      decorator_ClusterEnergy_maxEnergyLayer (*track) = std::vector<int>();

      decorator_CellEnergy_Energy (*track) = std::vector<float>();
      decorator_CellEnergy_Eta (*track) = std::vector<float>();
      decorator_CellEnergy_Phi (*track) = std::vector<float>();
      decorator_CellEnergy_emProbability (*track) = std::vector<float>();
      decorator_CellEnergy_firstEnergyDensity (*track) = std::vector<float>();
      decorator_CellEnergy_dRToTrack (*track) = std::vector<float>();
      decorator_CellEnergy_lambdaCenter (*track) = std::vector<float>();
      decorator_CellEnergy_maxEnergyLayer (*track) = std::vector<int>();

      decorator_ClusterEnergyLCW_Energy (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Eta (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Phi (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_emProbability (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_firstEnergyDensity (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_dRToTrack (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_lambdaCenter (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_maxEnergyLayer (*track) = std::vector<int>();

      decorator_trkEta_PreSamplerB (*track) = -999999999;
      decorator_trkPhi_PreSamplerB (*track) = -999999999;
      decorator_trkEta_PreSamplerE (*track) = -999999999;
      decorator_trkPhi_PreSamplerE (*track) = -999999999;

      decorator_trkEta_EMB1 (*track) = -999999999;
      decorator_trkPhi_EMB1 (*track) = -999999999;
      decorator_trkEta_EMB2 (*track) = -999999999;
      decorator_trkPhi_EMB2 (*track) = -999999999;
      decorator_trkEta_EMB3 (*track) = -999999999;
      decorator_trkPhi_EMB3 (*track) = -999999999;

      decorator_trkEta_EME1 (*track) = -999999999;
      decorator_trkPhi_EME1 (*track) = -999999999;
      decorator_trkEta_EME2 (*track) = -999999999;
      decorator_trkPhi_EME2 (*track) = -999999999;
      decorator_trkEta_EME3 (*track) = -999999999;
      decorator_trkPhi_EME3 (*track) = -999999999;

      decorator_trkEta_HEC0 (*track) = -999999999;
      decorator_trkPhi_HEC0 (*track) = -999999999;
      decorator_trkEta_HEC1 (*track) = -999999999;
      decorator_trkPhi_HEC1 (*track) = -999999999;
      decorator_trkEta_HEC2 (*track) = -999999999;
      decorator_trkPhi_HEC2 (*track) = -999999999;
      decorator_trkEta_HEC3 (*track) = -999999999;
      decorator_trkPhi_HEC3 (*track) = -999999999;

      decorator_trkEta_TileBar0 (*track) = -999999999;
      decorator_trkPhi_TileBar0 (*track) = -999999999; 
      decorator_trkEta_TileBar1 (*track) = -999999999;
      decorator_trkPhi_TileBar1 (*track) = -999999999;
      decorator_trkEta_TileBar2 (*track) = -999999999;
      decorator_trkPhi_TileBar2 (*track) = -999999999;

      decorator_trkEta_TileGap1 (*track) = -999999999;
      decorator_trkPhi_TileGap1 (*track) = -999999999;
      decorator_trkEta_TileGap2 (*track) = -999999999;
      decorator_trkPhi_TileGap2 (*track) = -999999999;
      decorator_trkEta_TileGap3 (*track) = -999999999;
      decorator_trkPhi_TileGap3 (*track) = -999999999;

      decorator_trkEta_TileExt0 (*track) = -999999999;
      decorator_trkPhi_TileExt0 (*track) = -999999999;
      decorator_trkEta_TileExt1 (*track) = -999999999;
      decorator_trkPhi_TileExt1 (*track) = -999999999;
      decorator_trkEta_TileExt2 (*track) = -999999999;
      decorator_trkPhi_TileExt2 (*track) = -999999999;

      decorator_ClusterEnergy_PreSamplerB_200 (*track) = -999999999;
      decorator_ClusterEnergy_PreSamplerB_100 (*track) = -999999999;
      decorator_ClusterEnergy_PreSamplerE_200 (*track) = -999999999;
      decorator_ClusterEnergy_PreSamplerE_100 (*track) = -999999999;

      decorator_ClusterEnergy_EMB1_200 (*track) = -999999999;
      decorator_ClusterEnergy_EMB1_100 (*track) = -999999999;
      decorator_ClusterEnergy_EMB2_200 (*track) = -999999999;
      decorator_ClusterEnergy_EMB2_100 (*track) = -999999999;
      decorator_ClusterEnergy_EMB3_200 (*track) = -999999999;
      decorator_ClusterEnergy_EMB3_100 (*track) = -999999999;

      decorator_ClusterEnergy_EME1_200(*track) = -999999999;
      decorator_ClusterEnergy_EME1_100(*track) = -999999999;
      decorator_ClusterEnergy_EME2_200(*track) = -999999999;
      decorator_ClusterEnergy_EME2_100(*track) = -999999999;
      decorator_ClusterEnergy_EME3_200(*track) = -999999999;
      decorator_ClusterEnergy_EME3_100(*track) = -999999999;

      decorator_ClusterEnergy_HEC0_200(*track) = -999999999;
      decorator_ClusterEnergy_HEC0_100(*track) = -999999999;
      decorator_ClusterEnergy_HEC1_200(*track) = -999999999;
      decorator_ClusterEnergy_HEC1_100(*track) = -999999999;
      decorator_ClusterEnergy_HEC2_200(*track) = -999999999;
      decorator_ClusterEnergy_HEC2_100(*track) = -999999999;
      decorator_ClusterEnergy_HEC3_200(*track) = -999999999;
      decorator_ClusterEnergy_HEC3_100(*track) = -999999999;

      decorator_ClusterEnergy_TileBar0_200(*track) = -999999999;
      decorator_ClusterEnergy_TileBar0_100(*track) = -999999999;
      decorator_ClusterEnergy_TileBar1_200(*track) = -999999999;
      decorator_ClusterEnergy_TileBar1_100(*track) = -999999999;
      decorator_ClusterEnergy_TileBar2_200(*track) = -999999999;
      decorator_ClusterEnergy_TileBar2_100(*track) = -999999999;

      decorator_ClusterEnergy_TileGap1_200(*track) = -999999999;
      decorator_ClusterEnergy_TileGap1_100(*track) = -999999999;
      decorator_ClusterEnergy_TileGap2_200(*track) = -999999999;
      decorator_ClusterEnergy_TileGap2_100(*track) = -999999999;
      decorator_ClusterEnergy_TileGap3_200(*track) = -999999999;
      decorator_ClusterEnergy_TileGap3_100(*track) = -999999999;

      decorator_ClusterEnergy_TileExt0_200(*track) = -999999999;
      decorator_ClusterEnergy_TileExt0_100(*track) = -999999999;
      decorator_ClusterEnergy_TileExt1_200(*track) = -999999999;
      decorator_ClusterEnergy_TileExt1_100(*track) = -999999999;
      decorator_ClusterEnergy_TileExt2_200(*track) = -999999999;
      decorator_ClusterEnergy_TileExt2_100(*track) = -999999999;

      decorator_EM_ClusterEnergy_0_200(*track) = -999999999;
      decorator_EM_ClusterEnergy_0_100(*track) = -999999999;
      decorator_HAD_ClusterEnergy_0_200(*track) = -999999999;
      decorator_HAD_ClusterEnergy_0_100(*track) = -999999999;
      decorator_Total_ClusterEnergy_0_200(*track) = -999999999;
      decorator_Total_ClusterEnergy_0_100(*track) = -999999999;

      decorator_ClusterEnergyLCW_PreSamplerB_200 (*track) = -999999999;
      decorator_ClusterEnergyLCW_PreSamplerB_100 (*track) = -999999999;
      decorator_ClusterEnergyLCW_PreSamplerE_200 (*track) = -999999999;
      decorator_ClusterEnergyLCW_PreSamplerE_100 (*track) = -999999999;

      decorator_ClusterEnergyLCW_EMB1_200 (*track) = -999999999;
      decorator_ClusterEnergyLCW_EMB1_100 (*track) = -999999999;
      decorator_ClusterEnergyLCW_EMB2_200 (*track) = -999999999;
      decorator_ClusterEnergyLCW_EMB2_100 (*track) = -999999999;
      decorator_ClusterEnergyLCW_EMB3_200 (*track) = -999999999;
      decorator_ClusterEnergyLCW_EMB3_100 (*track) = -999999999;

      decorator_ClusterEnergyLCW_EME1_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_EME1_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_EME2_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_EME2_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_EME3_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_EME3_100(*track) = -999999999;

      decorator_ClusterEnergyLCW_HEC0_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC0_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC1_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC1_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC2_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC2_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC3_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_HEC3_100(*track) = -999999999;

      decorator_ClusterEnergyLCW_TileBar0_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileBar0_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileBar1_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileBar1_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileBar2_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileBar2_100(*track) = -999999999;

      decorator_ClusterEnergyLCW_TileGap1_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileGap1_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileGap2_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileGap2_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileGap3_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileGap3_100(*track) = -999999999;

      decorator_ClusterEnergyLCW_TileExt0_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileExt0_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileExt1_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileExt1_100(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileExt2_200(*track) = -999999999;
      decorator_ClusterEnergyLCW_TileExt2_100(*track) = -999999999;

      decorator_EM_ClusterEnergyLCW_0_200(*track) = -999999999;
      decorator_EM_ClusterEnergyLCW_0_100(*track) = -999999999;
      decorator_HAD_ClusterEnergyLCW_0_200(*track) = -999999999;
      decorator_HAD_ClusterEnergyLCW_0_100(*track) = -999999999;
      decorator_Total_ClusterEnergyLCW_0_200(*track) = -999999999;
      decorator_Total_ClusterEnergyLCW_0_100(*track) = -999999999;

      decorator_CellEnergy_PreSamplerB_200 (*track) = -999999999;
      decorator_CellEnergy_PreSamplerB_100 (*track) = -999999999;
      decorator_CellEnergy_PreSamplerE_200 (*track) = -999999999;
      decorator_CellEnergy_PreSamplerE_100 (*track) = -999999999;

      decorator_CellEnergy_EMB1_200 (*track) = -999999999;
      decorator_CellEnergy_EMB1_100 (*track) = -999999999;
      decorator_CellEnergy_EMB2_200 (*track) = -999999999;
      decorator_CellEnergy_EMB2_100 (*track) = -999999999;
      decorator_CellEnergy_EMB3_200 (*track) = -999999999;
      decorator_CellEnergy_EMB3_100 (*track) = -999999999;

      decorator_CellEnergy_EME1_200(*track) = -999999999;
      decorator_CellEnergy_EME1_100(*track) = -999999999;
      decorator_CellEnergy_EME2_200(*track) = -999999999;
      decorator_CellEnergy_EME2_100(*track) = -999999999;
      decorator_CellEnergy_EME3_200(*track) = -999999999;
      decorator_CellEnergy_EME3_100(*track) = -999999999;

      decorator_CellEnergy_HEC0_200(*track) = -999999999;
      decorator_CellEnergy_HEC0_100(*track) = -999999999;
      decorator_CellEnergy_HEC1_200(*track) = -999999999;
      decorator_CellEnergy_HEC1_100(*track) = -999999999;
      decorator_CellEnergy_HEC2_200(*track) = -999999999;
      decorator_CellEnergy_HEC2_100(*track) = -999999999;
      decorator_CellEnergy_HEC3_200(*track) = -999999999;
      decorator_CellEnergy_HEC3_100(*track) = -999999999;

      decorator_CellEnergy_TileBar0_200(*track) = -999999999;
      decorator_CellEnergy_TileBar0_100(*track) = -999999999;
      decorator_CellEnergy_TileBar1_200(*track) = -999999999;
      decorator_CellEnergy_TileBar1_100(*track) = -999999999;
      decorator_CellEnergy_TileBar2_200(*track) = -999999999;
      decorator_CellEnergy_TileBar2_100(*track) = -999999999;

      decorator_CellEnergy_TileGap1_200(*track) = -999999999;
      decorator_CellEnergy_TileGap1_100(*track) = -999999999;
      decorator_CellEnergy_TileGap2_200(*track) = -999999999;
      decorator_CellEnergy_TileGap2_100(*track) = -999999999;
      decorator_CellEnergy_TileGap3_200(*track) = -999999999;
      decorator_CellEnergy_TileGap3_100(*track) = -999999999;

      decorator_CellEnergy_TileExt0_200(*track) = -999999999;
      decorator_CellEnergy_TileExt0_100(*track) = -999999999;
      decorator_CellEnergy_TileExt1_200(*track) = -999999999;
      decorator_CellEnergy_TileExt1_100(*track) = -999999999;
      decorator_CellEnergy_TileExt2_200(*track) = -999999999;
      decorator_CellEnergy_TileExt2_100(*track) = -999999999;

      decorator_EM_CellEnergy_0_200(*track) = -999999999;
      decorator_EM_CellEnergy_0_100(*track) = -999999999;
      decorator_HAD_CellEnergy_0_200(*track) = -999999999;
      decorator_HAD_CellEnergy_0_100(*track) = -999999999;
      decorator_Total_CellEnergy_0_200(*track) = -999999999;
      decorator_Total_CellEnergy_0_100(*track) = -999999999;

      /*a map to store the track parameters associated with the different layers of the calorimeter system */
      std::map<CaloCell_ID::CaloSample, const Trk::TrackParameters*> parametersMap;

      /*get the CaloExtension object*/
      const Trk::CaloExtension* extension = 0;

      if (m_theTrackExtrapolatorTool->caloExtension(*track, extension, false)) {

        /*extract the CurvilinearParameters per each layer-track intersection*/
        const std::vector<const Trk::CurvilinearParameters*>& clParametersVector = extension->caloLayerIntersections();

        for (auto clParameter : clParametersVector) {

          unsigned int parametersIdentifier = clParameter->cIdentifier();
          CaloCell_ID::CaloSample intLayer;

          if (!m_trackParametersIdHelper->isValid(parametersIdentifier)) {
            std::cout << "Invalid Track Identifier"<< std::endl;
            intLayer = CaloCell_ID::CaloSample::Unknown;
          } else {
            intLayer = m_trackParametersIdHelper->caloSample(parametersIdentifier);
          }

          if (parametersMap[intLayer] == NULL) {
            parametersMap[intLayer] = clParameter->clone();
          } else if (m_trackParametersIdHelper->isEntryToVolume(clParameter->cIdentifier())) {
            delete parametersMap[intLayer];
            parametersMap[intLayer] = clParameter->clone();
          }
        }

      } else {
        //msg(MSG::WARNING) << "TrackExtension failed for track with pt and eta " << track->pt() << " and " << track->eta() << endreq;
      }

      if(!(m_theTrackExtrapolatorTool->caloExtension(*track, extension, false))) continue; //No valid parameters for any of the layers of interest
      decorator_extrapolation(*track) = 1;

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_extrapolation, 1);

      /*Decorate track with extended eta and phi coordinates at intersection layers*/
      if(parametersMap[CaloCell_ID::CaloSample::PreSamplerB]) decorator_trkEta_PreSamplerB(*track) = parametersMap[CaloCell_ID::CaloSample::PreSamplerB]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::PreSamplerB]) decorator_trkPhi_PreSamplerB(*track) = parametersMap[CaloCell_ID::CaloSample::PreSamplerB]->position().phi();  
      if(parametersMap[CaloCell_ID::CaloSample::PreSamplerE]) decorator_trkEta_PreSamplerE(*track) = parametersMap[CaloCell_ID::CaloSample::PreSamplerE]->position().eta();    
      if(parametersMap[CaloCell_ID::CaloSample::PreSamplerE]) decorator_trkPhi_PreSamplerE(*track) = parametersMap[CaloCell_ID::CaloSample::PreSamplerE]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::EMB1]) decorator_trkEta_EMB1(*track) = parametersMap[CaloCell_ID::CaloSample::EMB1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EMB1]) decorator_trkPhi_EMB1(*track) = parametersMap[CaloCell_ID::CaloSample::EMB1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::EMB2]) decorator_trkEta_EMB2(*track) = parametersMap[CaloCell_ID::CaloSample::EMB2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EMB2]) decorator_trkPhi_EMB2(*track) = parametersMap[CaloCell_ID::CaloSample::EMB2]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::EMB3]) decorator_trkEta_EMB3(*track) = parametersMap[CaloCell_ID::CaloSample::EMB3]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EMB3]) decorator_trkPhi_EMB3(*track) = parametersMap[CaloCell_ID::CaloSample::EMB3]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::EME1]) decorator_trkEta_EME1(*track) = parametersMap[CaloCell_ID::CaloSample::EME1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EME1]) decorator_trkPhi_EME1(*track) = parametersMap[CaloCell_ID::CaloSample::EME1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::EME2]) decorator_trkEta_EME2(*track) = parametersMap[CaloCell_ID::CaloSample::EME2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EME2]) decorator_trkPhi_EME2(*track) = parametersMap[CaloCell_ID::CaloSample::EME2]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::EME3]) decorator_trkEta_EME3(*track) = parametersMap[CaloCell_ID::CaloSample::EME3]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::EME3]) decorator_trkPhi_EME3(*track) = parametersMap[CaloCell_ID::CaloSample::EME3]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::HEC0]) decorator_trkEta_HEC0 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC0]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::HEC0]) decorator_trkPhi_HEC0 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC0]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::HEC1]) decorator_trkEta_HEC1 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::HEC1]) decorator_trkPhi_HEC1 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::HEC2]) decorator_trkEta_HEC2 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::HEC2]) decorator_trkPhi_HEC2 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC2]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::HEC3]) decorator_trkEta_HEC3 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC3]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::HEC3]) decorator_trkPhi_HEC3 (*track) = parametersMap[CaloCell_ID::CaloSample::HEC3]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::TileBar0]) decorator_trkEta_TileBar0 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar0]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileBar0]) decorator_trkPhi_TileBar0 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar0]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileBar1]) decorator_trkEta_TileBar1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileBar1]) decorator_trkPhi_TileBar1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileBar2]) decorator_trkEta_TileBar2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileBar2]) decorator_trkPhi_TileBar2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileBar2]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::TileGap1]) decorator_trkEta_TileGap1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileGap1]) decorator_trkPhi_TileGap1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileGap2]) decorator_trkEta_TileGap2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileGap2]) decorator_trkPhi_TileGap2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap2]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileGap3]) decorator_trkEta_TileGap3 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap3]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileGap3]) decorator_trkPhi_TileGap3 (*track) = parametersMap[CaloCell_ID::CaloSample::TileGap3]->position().phi();

      if(parametersMap[CaloCell_ID::CaloSample::TileExt0]) decorator_trkEta_TileExt0 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt0]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileExt0]) decorator_trkPhi_TileExt0 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt0]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileExt1]) decorator_trkEta_TileExt1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt1]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileExt1]) decorator_trkPhi_TileExt1 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt1]->position().phi();
      if(parametersMap[CaloCell_ID::CaloSample::TileExt2]) decorator_trkEta_TileExt2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt2]->position().eta();
      if(parametersMap[CaloCell_ID::CaloSample::TileExt2]) decorator_trkPhi_TileExt2 (*track) = parametersMap[CaloCell_ID::CaloSample::TileExt2]->position().phi();

      /*Track-cluster matching*/
      //Approach: find the most energetic layer of a cluster. Record the eta and phi coordinates of the extrapolated track at this layer//
      //Perform the matching between these track eta and phi coordinates and the energy-weighted (bary)centre eta and phi of the cluster//

      int matchedClusterCounter_200 = 0;
      int matchedClusterCounter_100 = 0;
      std::vector<xAOD::CaloCluster*> matchedClusters_200; //clusters within DeltaR < 0.2 of the track, reinitialised for each layer
      std::vector<xAOD::CaloCluster*> matchedClusters_100; //clusters within DeltaR < 0.1 of the track

      std::vector<float> ClusterEnergy_Energy;
      std::vector<float> ClusterEnergy_Eta;
      std::vector<float> ClusterEnergy_Phi;
      std::vector<float> ClusterEnergy_dRToTrack;
      std::vector<float> ClusterEnergy_lambdaCenter;
      std::vector<float> ClusterEnergy_firstEnergyDensity;
      std::vector<float> ClusterEnergy_emProbability;
      std::vector<int> ClusterEnergy_maxEnergyLayer;

      std::vector<float> ClusterEnergyLCW_Energy;
      std::vector<float> ClusterEnergyLCW_Eta;
      std::vector<float> ClusterEnergyLCW_Phi;
      std::vector<float> ClusterEnergyLCW_dRToTrack;
      std::vector<float> ClusterEnergyLCW_lambdaCenter;
      std::vector<float> ClusterEnergyLCW_firstEnergyDensity;
      std::vector<float> ClusterEnergyLCW_emProbability;
      std::vector<int> ClusterEnergyLCW_maxEnergyLayer;

      std::vector<float> CellEnergy_Energy;
      std::vector<float> CellEnergy_Eta;
      std::vector<float> CellEnergy_Phi;
      std::vector<float> CellEnergy_dRToTrack;
      std::vector<float> CellEnergy_lambdaCenter;
      std::vector<float> CellEnergy_firstEnergyDensity;
      std::vector<float> CellEnergy_emProbability;
      std::vector<int> CellEnergy_maxEnergyLayer;

      for (const auto& cluster : *clusterContainer) {

        /*Finding the most energetic layer of the cluster*/
        xAOD::CaloCluster::CaloSample mostEnergeticLayer = xAOD::CaloCluster::CaloSample::Unknown;
        double maxLayerClusterEnergy = -999999999; //Some extremely low value

        for (int i=0; i<xAOD::CaloCluster::CaloSample::TileExt2+1; i++) {

          xAOD::CaloCluster::CaloSample sampleLayer = (xAOD::CaloCluster::CaloSample)(i);
          double clusterLayerEnergy = cluster->eSample(sampleLayer);

          if(clusterLayerEnergy > maxLayerClusterEnergy) {
            maxLayerClusterEnergy = clusterLayerEnergy;
            mostEnergeticLayer = sampleLayer;
          }
        }

        if(mostEnergeticLayer==xAOD::CaloCluster::CaloSample::Unknown) continue;

        //do track-cluster matching at EM-Scale
        double clEta = cluster->rawEta();
        double clPhi = cluster->rawPhi();

        if(clEta == -999 || clPhi == -999) continue;

        /*Matching between the track parameters in the most energetic layer and the cluster barycentre*/

        if(!parametersMap[mostEnergeticLayer]) continue;

        double trackEta = parametersMap[mostEnergeticLayer]->position().eta();
        double trackPhi = parametersMap[mostEnergeticLayer]->position().phi();

        double etaDiff = clEta - trackEta;
        double phiDiff = clPhi - trackPhi;

        if (phiDiff > TMath::Pi()) phiDiff = 2 * TMath::Pi() - phiDiff;

        double deltaR = std::sqrt((etaDiff*etaDiff) + (phiDiff*phiDiff));

        if(deltaR < 0.2){
          matchedClusterCounter_200++;
          matchedClusters_200.push_back(const_cast<xAOD::CaloCluster*>(cluster));

          //decorate with EM-scale quanitities.
          double lambda_center;
          double em_probability;
          double first_energy_density;
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 501, lambda_center)) {ATH_MSG_WARNING("Couldn't retrieve the cluster lambda center");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 900, em_probability)) {ATH_MSG_WARNING("Couldn't rertieve the EM Probability");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 804, first_energy_density)) {ATH_MSG_WARNING("Couldn't rertieve the first energy density moment");}

          //we want to include the information about these clusters in the derivation output
          ClusterEnergy_Energy.push_back(cluster->rawE()); //Raw Energy
          ClusterEnergy_Eta.push_back(cluster->rawEta()); //Eta and phi based on EM Scale
          ClusterEnergy_Phi.push_back(cluster->rawPhi()); //Eta and phi based on EM Scale
          ClusterEnergy_dRToTrack.push_back(deltaR);
          ClusterEnergy_lambdaCenter.push_back(lambda_center);
          ClusterEnergy_maxEnergyLayer.push_back(mostEnergeticLayer);
          ClusterEnergy_emProbability.push_back(em_probability);
          ClusterEnergy_firstEnergyDensity.push_back(first_energy_density);
          
          ClusterEnergyLCW_Energy.push_back(cluster->e());   //LCW Energy
          ClusterEnergyLCW_Eta.push_back(cluster->calEta()); // Eta and phi at LCW scaleScale
          ClusterEnergyLCW_Phi.push_back(cluster->calPhi()); 
          ClusterEnergyLCW_dRToTrack.push_back(deltaR);
          ClusterEnergyLCW_lambdaCenter.push_back(lambda_center);
          ClusterEnergyLCW_maxEnergyLayer.push_back(mostEnergeticLayer);
          ClusterEnergyLCW_emProbability.push_back(em_probability);
          ClusterEnergyLCW_firstEnergyDensity.push_back(first_energy_density);
        }

        if(deltaR < 0.1){
          matchedClusterCounter_100++;
          matchedClusters_100.push_back(const_cast<xAOD::CaloCluster*>(cluster));
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_cluster_matching, 1);

      /*Track-cell matching*/
      //Approach: loop over cell container, getting the eta and phi coordinates of each cell for each layer.//
      //Perform a match between the cell and the track eta and phi coordinates in the cell's sampling layer.//

      int matchedCellCounter_200 = 0;
      int matchedCellCounter_100 = 0;
      std::vector<CaloCell*> matchedCells_200;
      std::vector<CaloCell*> matchedCells_100;

      for (const auto& cell : *caloCellContainer) {

        if (!cell->caloDDE()) continue;

        CaloCell_ID::CaloSample cellLayer = cell->caloDDE()->getSampling();

        if(cellLayer>CaloCell_ID::CaloSample::TileExt2) continue;

        double cellEta = cell->eta();
        double cellPhi = cell->phi();

        if(cellEta == -999 || cellPhi == -999) continue;

        if(!parametersMap[cellLayer]) continue;

        double trackEta = parametersMap[cellLayer]->position().eta();
        double trackPhi = parametersMap[cellLayer]->position().phi();

        double etaDiff = cellEta - trackEta;
        double phiDiff = cellPhi - trackPhi;

        if (phiDiff > TMath::Pi()) phiDiff = 2*TMath::Pi() - phiDiff;

        double deltaR = std::sqrt((etaDiff*etaDiff) + (phiDiff*phiDiff));

        if (deltaR < 0.2){
          matchedCellCounter_200++;
          matchedCells_200.push_back(const_cast<CaloCell*>(cell));
          //We want to include information about these clusters in the derivation output
          //CellEnergy_Energy.push_back(cell->e()); This is too much information to include in the output derivation
          //CellEnergy_Eta.push_back(cellEta);
          //CellEnergy_Phi.push_back(cellPhi);
          //CellEnergy_dRToTrack.push_back(deltaR);
          //CellEnergy_MaxEnergyLayer.push_back(cellLayer);
        }

        if (deltaR < 0.1){
          matchedCellCounter_100++;
          matchedCells_100.push_back(const_cast<CaloCell*>(cell));
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_cell_matching, 1);

      /*Populating cluster decorations*/
      std::vector<std::vector<xAOD::CaloCluster*>> matchedClusters;
      matchedClusters.push_back(matchedClusters_200);
      matchedClusters.push_back(matchedClusters_100);

      int scales = 2;

      /*Loop over scales, where j=0 is the EM scale and j=1 is the LCW scale*/
      for (int j=0; j < scales; j++) {

        double clusterEnergy[2][21] = {{0}};

        double totalEMBClusterEnergy[2] = {};
        double totalEMEClusterEnergy[2] = {};
        double totalEMClusterEnergy[2] = {};

        double totalHECClusterEnergy[2] = {};
        double totalTileBarClusterEnergy[2] = {};
        double totalTileGapClusterEnergy[2] = {};
        double totalTileExtClusterEnergy[2] = {};
        double totalHADClusterEnergy[2] = {};

        double totalClusterEnergy[2] = {};

        /*Loop over cone dimensions, where i=0 is DeltaR < 0.2 and i=1 is DeltaR < 0.1*/
        for (unsigned int i=0; i < matchedClusters.size(); i++) {

          double totalEnergy = 0;

          std::vector<xAOD::CaloCluster*>::iterator firstMatchedClus = matchedClusters[i].begin();
          std::vector<xAOD::CaloCluster*>::iterator lastMatchedClus = matchedClusters[i].end();

          /*Loop over matched clusters for a given cone dimension*/
          for (; firstMatchedClus != lastMatchedClus; ++firstMatchedClus) {

            xAOD::CaloCluster& cl = **firstMatchedClus;
            double energy_EM = -999999999;
            double energy_LCW = -999999999;

            energy_EM = cl.rawE();
            energy_LCW = cl.calE();

            if(energy_EM == -999999999 || energy_LCW == -999999999) continue;

            double cluster_weight = energy_LCW/energy_EM;

            if(j==0) totalEnergy += (*firstMatchedClus)->rawE(); //EM scale energy
            if(j==1) totalEnergy += (*firstMatchedClus)->e(); //LCW scale energy

            for(unsigned int m=0; m<CaloCell_ID::CaloSample::TileExt2+1; m++) {

              xAOD::CaloCluster::CaloSample clusterLayer = (xAOD::CaloCluster::CaloSample)(m);

              if(j==0) {
                clusterEnergy[i][m] += (*firstMatchedClus)->eSample(clusterLayer); //eSample returns EM scale energy only
              }
              if(j==1) {
                clusterEnergy[i][m] += cluster_weight*((*firstMatchedClus)->eSample(clusterLayer)); 
              }
            }  
          }

          totalEMBClusterEnergy[i] = clusterEnergy[i][1] + clusterEnergy[i][2] + clusterEnergy[i][3];
          totalEMEClusterEnergy[i] = clusterEnergy[i][5] + clusterEnergy[i][6] + clusterEnergy[i][7];
          totalEMClusterEnergy[i] = totalEMBClusterEnergy[i] + totalEMEClusterEnergy[i];

          totalHECClusterEnergy[i] = clusterEnergy[i][8] + clusterEnergy[i][9] + clusterEnergy[i][10] + clusterEnergy[i][11];
          totalTileBarClusterEnergy[i] = clusterEnergy[i][12] + clusterEnergy[i][13] + clusterEnergy[i][14];
          totalTileGapClusterEnergy[i] = clusterEnergy[i][15] + clusterEnergy[i][16] + clusterEnergy[i][17];
          totalTileExtClusterEnergy[i] = clusterEnergy[i][18] + clusterEnergy[i][19] + clusterEnergy[i][20];
          totalHADClusterEnergy[i] = totalHECClusterEnergy[i] + totalTileBarClusterEnergy[i] + totalTileGapClusterEnergy[i] + totalTileExtClusterEnergy[i];

          totalClusterEnergy[i] = totalEnergy;

        }

        if(j==0) {
          /*Cluster decorations at EM scale*/
          decorator_ClusterEnergy_PreSamplerB_200(*track) = clusterEnergy[0][0];
          decorator_ClusterEnergy_PreSamplerB_100(*track) = clusterEnergy[1][0];
          decorator_ClusterEnergy_PreSamplerE_200(*track) = clusterEnergy[0][4];
          decorator_ClusterEnergy_PreSamplerE_100(*track) = clusterEnergy[1][4];

          decorator_ClusterEnergy_EMB1_200(*track) = clusterEnergy[0][1];
          decorator_ClusterEnergy_EMB1_100(*track) = clusterEnergy[1][1];
          decorator_ClusterEnergy_EMB2_200(*track) = clusterEnergy[0][2];
          decorator_ClusterEnergy_EMB2_100(*track) = clusterEnergy[1][2];
          decorator_ClusterEnergy_EMB3_200(*track) = clusterEnergy[0][3];
          decorator_ClusterEnergy_EMB3_100(*track) = clusterEnergy[1][3];

          decorator_ClusterEnergy_EME1_200(*track) = clusterEnergy[0][5];
          decorator_ClusterEnergy_EME1_100(*track) = clusterEnergy[1][5];
          decorator_ClusterEnergy_EME2_200(*track) = clusterEnergy[0][6];
          decorator_ClusterEnergy_EME2_100(*track) = clusterEnergy[1][6];
          decorator_ClusterEnergy_EME3_200(*track) = clusterEnergy[0][7];
          decorator_ClusterEnergy_EME3_100(*track) = clusterEnergy[1][7];

          decorator_ClusterEnergy_HEC0_200(*track) = clusterEnergy[0][8];
          decorator_ClusterEnergy_HEC0_100(*track) = clusterEnergy[1][8];
          decorator_ClusterEnergy_HEC1_200(*track) = clusterEnergy[0][9];
          decorator_ClusterEnergy_HEC1_100(*track) = clusterEnergy[1][9];
          decorator_ClusterEnergy_HEC2_200(*track) = clusterEnergy[0][10];
          decorator_ClusterEnergy_HEC2_100(*track) = clusterEnergy[1][10];
          decorator_ClusterEnergy_HEC3_200(*track) = clusterEnergy[0][11];
          decorator_ClusterEnergy_HEC3_100(*track) = clusterEnergy[1][11];

          decorator_ClusterEnergy_TileBar0_200(*track) = clusterEnergy[0][12];
          decorator_ClusterEnergy_TileBar0_100(*track) = clusterEnergy[1][12];
          decorator_ClusterEnergy_TileBar1_200(*track) = clusterEnergy[0][13];
          decorator_ClusterEnergy_TileBar1_100(*track) = clusterEnergy[1][13];
          decorator_ClusterEnergy_TileBar2_200(*track) = clusterEnergy[0][14];
          decorator_ClusterEnergy_TileBar2_100(*track) = clusterEnergy[1][14];

          decorator_ClusterEnergy_TileGap1_200(*track) = clusterEnergy[0][15];
          decorator_ClusterEnergy_TileGap1_100(*track) = clusterEnergy[1][15];
          decorator_ClusterEnergy_TileGap2_200(*track) = clusterEnergy[0][16];
          decorator_ClusterEnergy_TileGap2_100(*track) = clusterEnergy[1][16];
          decorator_ClusterEnergy_TileGap3_200(*track) = clusterEnergy[0][17];
          decorator_ClusterEnergy_TileGap3_100(*track) = clusterEnergy[1][17];

          decorator_ClusterEnergy_TileExt0_200(*track) = clusterEnergy[0][18];
          decorator_ClusterEnergy_TileExt0_100(*track) = clusterEnergy[1][18];
          decorator_ClusterEnergy_TileExt1_200(*track) = clusterEnergy[0][19];
          decorator_ClusterEnergy_TileExt1_100(*track) = clusterEnergy[1][19];
          decorator_ClusterEnergy_TileExt2_200(*track) = clusterEnergy[0][20];
          decorator_ClusterEnergy_TileExt2_100(*track) = clusterEnergy[1][20];

          decorator_EM_ClusterEnergy_0_200(*track) = totalEMClusterEnergy[0];
          decorator_EM_ClusterEnergy_0_100(*track) = totalEMClusterEnergy[1];
          decorator_HAD_ClusterEnergy_0_200(*track) = totalHADClusterEnergy[0];
          decorator_HAD_ClusterEnergy_0_100(*track) = totalHADClusterEnergy[1];
          decorator_Total_ClusterEnergy_0_200(*track) = totalClusterEnergy[0];
          decorator_Total_ClusterEnergy_0_100(*track) = totalClusterEnergy[1];	

        } else if (j==1) {
          /*Cluster decorations at LCW scale*/
          decorator_ClusterEnergyLCW_PreSamplerB_200(*track) = clusterEnergy[0][0];
          decorator_ClusterEnergyLCW_PreSamplerB_100(*track) = clusterEnergy[1][0];
          decorator_ClusterEnergyLCW_PreSamplerE_200(*track) = clusterEnergy[0][4];
          decorator_ClusterEnergyLCW_PreSamplerE_100(*track) = clusterEnergy[1][4];

          decorator_ClusterEnergyLCW_EMB1_200(*track) = clusterEnergy[0][1];
          decorator_ClusterEnergyLCW_EMB1_100(*track) = clusterEnergy[1][1];
          decorator_ClusterEnergyLCW_EMB2_200(*track) = clusterEnergy[0][2];
          decorator_ClusterEnergyLCW_EMB2_100(*track) = clusterEnergy[1][2];
          decorator_ClusterEnergyLCW_EMB3_200(*track) = clusterEnergy[0][3];
          decorator_ClusterEnergyLCW_EMB3_100(*track) = clusterEnergy[1][3];

          decorator_ClusterEnergyLCW_EME1_200(*track) = clusterEnergy[0][5];
          decorator_ClusterEnergyLCW_EME1_100(*track) = clusterEnergy[1][5];
          decorator_ClusterEnergyLCW_EME2_200(*track) = clusterEnergy[0][6];
          decorator_ClusterEnergyLCW_EME2_100(*track) = clusterEnergy[1][6];
          decorator_ClusterEnergyLCW_EME3_200(*track) = clusterEnergy[0][7];
          decorator_ClusterEnergyLCW_EME3_100(*track) = clusterEnergy[1][7];

          decorator_ClusterEnergyLCW_HEC0_200(*track) = clusterEnergy[0][8];
          decorator_ClusterEnergyLCW_HEC0_100(*track) = clusterEnergy[1][8];
          decorator_ClusterEnergyLCW_HEC1_200(*track) = clusterEnergy[0][9];
          decorator_ClusterEnergyLCW_HEC1_100(*track) = clusterEnergy[1][9];
          decorator_ClusterEnergyLCW_HEC2_200(*track) = clusterEnergy[0][10];
          decorator_ClusterEnergyLCW_HEC2_100(*track) = clusterEnergy[1][10];
          decorator_ClusterEnergyLCW_HEC3_200(*track) = clusterEnergy[0][11];
          decorator_ClusterEnergyLCW_HEC3_100(*track) = clusterEnergy[1][11];

          decorator_ClusterEnergyLCW_TileBar0_200(*track) = clusterEnergy[0][12];
          decorator_ClusterEnergyLCW_TileBar0_100(*track) = clusterEnergy[1][12];
          decorator_ClusterEnergyLCW_TileBar1_200(*track) = clusterEnergy[0][13];
          decorator_ClusterEnergyLCW_TileBar1_100(*track) = clusterEnergy[1][13];
          decorator_ClusterEnergyLCW_TileBar2_200(*track) = clusterEnergy[0][14];
          decorator_ClusterEnergyLCW_TileBar2_100(*track) = clusterEnergy[1][14];

          decorator_ClusterEnergyLCW_TileGap1_200(*track) = clusterEnergy[0][15];
          decorator_ClusterEnergyLCW_TileGap1_100(*track) = clusterEnergy[1][15];
          decorator_ClusterEnergyLCW_TileGap2_200(*track) = clusterEnergy[0][16];
          decorator_ClusterEnergyLCW_TileGap2_100(*track) = clusterEnergy[1][16];
          decorator_ClusterEnergyLCW_TileGap3_200(*track) = clusterEnergy[0][17];
          decorator_ClusterEnergyLCW_TileGap3_100(*track) = clusterEnergy[1][17];

          decorator_ClusterEnergyLCW_TileExt0_200(*track) = clusterEnergy[0][18];
          decorator_ClusterEnergyLCW_TileExt0_100(*track) = clusterEnergy[1][18];
          decorator_ClusterEnergyLCW_TileExt1_200(*track) = clusterEnergy[0][19];
          decorator_ClusterEnergyLCW_TileExt1_100(*track) = clusterEnergy[1][19];
          decorator_ClusterEnergyLCW_TileExt2_200(*track) = clusterEnergy[0][20];
          decorator_ClusterEnergyLCW_TileExt2_100(*track) = clusterEnergy[1][20];

          decorator_EM_ClusterEnergyLCW_0_200(*track) = totalEMClusterEnergy[0];
          decorator_EM_ClusterEnergyLCW_0_100(*track) = totalEMClusterEnergy[1];
          decorator_HAD_ClusterEnergyLCW_0_200(*track) = totalHADClusterEnergy[0];
          decorator_HAD_ClusterEnergyLCW_0_100(*track) = totalHADClusterEnergy[1];
          decorator_Total_ClusterEnergyLCW_0_200(*track) = totalClusterEnergy[0];
          decorator_Total_ClusterEnergyLCW_0_100(*track) = totalClusterEnergy[1];

        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_loop_matched_clusters, 1);

      /*Populate cell decorations*/
      std::vector<std::vector<CaloCell*>> matchedCells;
      matchedCells.push_back(matchedCells_200);
      matchedCells.push_back(matchedCells_100);

      double matchedCellEnergy[2][21] = {{0}};

      double totalEMBCellEnergy[2] = {};
      double totalEMECellEnergy[2] = {};
      double totalEMCellEnergy[2] = {};

      double totalHECCellEnergy[2] = {};
      double totalTileBarCellEnergy[2] = {};
      double totalTileGapCellEnergy[2] = {};
      double totalTileExtCellEnergy[2] = {};
      double totalHADCellEnergy[2] = {};

      double totalCellEnergy[2] = {};

      for (unsigned int i=0; i < matchedCells.size(); i++) {

        double summedCellEnergy = 0;

        std::vector<CaloCell*>::iterator firstMatchedCell = matchedCells[i].begin();
        std::vector<CaloCell*>::iterator lastMatchedCell = matchedCells[i].end();

        for (; firstMatchedCell != lastMatchedCell; ++firstMatchedCell) {

          if (!(*firstMatchedCell)->caloDDE()) continue;

          summedCellEnergy += (*firstMatchedCell)->energy();

          CaloCell_ID::CaloSample cellLayer = (*firstMatchedCell)->caloDDE()->getSampling();

          if(cellLayer<=CaloCell_ID::CaloSample::TileExt2) matchedCellEnergy[i][cellLayer] += (*firstMatchedCell)->energy();
        }

        totalEMBCellEnergy[i] = matchedCellEnergy[i][1] + matchedCellEnergy[i][2] + matchedCellEnergy[i][3];
        totalEMECellEnergy[i] = matchedCellEnergy[i][5] + matchedCellEnergy[i][6] + matchedCellEnergy[i][7];
        totalEMCellEnergy[i] = totalEMBCellEnergy[i] + totalEMECellEnergy[i];

        totalHECCellEnergy[i] = matchedCellEnergy[i][8] + matchedCellEnergy[i][9] + matchedCellEnergy[i][10] + matchedCellEnergy[i][11];
        totalTileBarCellEnergy[i] = matchedCellEnergy[i][12] + matchedCellEnergy[i][13] + matchedCellEnergy[i][14];
        totalTileGapCellEnergy[i] = matchedCellEnergy[i][15] + matchedCellEnergy[i][16] + matchedCellEnergy[i][17];
        totalTileExtCellEnergy[i] = matchedCellEnergy[i][18] + matchedCellEnergy[i][19] + matchedCellEnergy[i][20];
        totalHADCellEnergy[i] = totalHECCellEnergy[i] + totalTileBarCellEnergy[i] + totalTileGapCellEnergy[i] + totalTileExtCellEnergy[i];

        totalCellEnergy[i] = summedCellEnergy;
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_loop_matched_cells, 1);

      /*Cell decorations*/
      decorator_CellEnergy_PreSamplerB_200(*track) = matchedCellEnergy[0][0];
      decorator_CellEnergy_PreSamplerB_100(*track) = matchedCellEnergy[1][0];
      decorator_CellEnergy_PreSamplerE_200(*track) = matchedCellEnergy[0][4];
      decorator_CellEnergy_PreSamplerE_100(*track) = matchedCellEnergy[1][4];

      decorator_CellEnergy_EMB1_200(*track) = matchedCellEnergy[0][1];
      decorator_CellEnergy_EMB1_100(*track) = matchedCellEnergy[1][1];
      decorator_CellEnergy_EMB2_200(*track) = matchedCellEnergy[0][2];
      decorator_CellEnergy_EMB2_100(*track) = matchedCellEnergy[1][2];
      decorator_CellEnergy_EMB3_200(*track) = matchedCellEnergy[0][3];
      decorator_CellEnergy_EMB3_100(*track) = matchedCellEnergy[1][3];

      decorator_CellEnergy_EME1_200(*track) = matchedCellEnergy[0][5];
      decorator_CellEnergy_EME1_100(*track) = matchedCellEnergy[1][5];
      decorator_CellEnergy_EME2_200(*track) = matchedCellEnergy[0][6];
      decorator_CellEnergy_EME2_100(*track) = matchedCellEnergy[1][6];
      decorator_CellEnergy_EME3_200(*track) = matchedCellEnergy[0][7];
      decorator_CellEnergy_EME3_100(*track) = matchedCellEnergy[1][7];

      decorator_CellEnergy_HEC0_200(*track) = matchedCellEnergy[0][8];
      decorator_CellEnergy_HEC0_100(*track) = matchedCellEnergy[1][8];
      decorator_CellEnergy_HEC1_200(*track) = matchedCellEnergy[0][9];
      decorator_CellEnergy_HEC1_100(*track) = matchedCellEnergy[1][9];
      decorator_CellEnergy_HEC2_200(*track) = matchedCellEnergy[0][10];
      decorator_CellEnergy_HEC2_100(*track) = matchedCellEnergy[1][10];
      decorator_CellEnergy_HEC3_200(*track) = matchedCellEnergy[0][11];
      decorator_CellEnergy_HEC3_100(*track) = matchedCellEnergy[1][11];

      decorator_CellEnergy_TileBar0_200(*track) = matchedCellEnergy[0][12];
      decorator_CellEnergy_TileBar0_100(*track) = matchedCellEnergy[1][12];
      decorator_CellEnergy_TileBar1_200(*track) = matchedCellEnergy[0][13];
      decorator_CellEnergy_TileBar1_100(*track) = matchedCellEnergy[1][13];
      decorator_CellEnergy_TileBar2_200(*track) = matchedCellEnergy[0][14];
      decorator_CellEnergy_TileBar2_100(*track) = matchedCellEnergy[1][14];

      decorator_CellEnergy_TileGap1_200(*track) = matchedCellEnergy[0][15];
      decorator_CellEnergy_TileGap1_100(*track) = matchedCellEnergy[1][15];
      decorator_CellEnergy_TileGap2_200(*track) = matchedCellEnergy[0][16];
      decorator_CellEnergy_TileGap2_100(*track) = matchedCellEnergy[1][16];
      decorator_CellEnergy_TileGap3_200(*track) = matchedCellEnergy[0][17];
      decorator_CellEnergy_TileGap3_100(*track) = matchedCellEnergy[1][17];

      decorator_CellEnergy_TileExt0_200(*track) = matchedCellEnergy[0][18];
      decorator_CellEnergy_TileExt0_100(*track) = matchedCellEnergy[1][18];
      decorator_CellEnergy_TileExt1_200(*track) = matchedCellEnergy[0][19];
      decorator_CellEnergy_TileExt1_100(*track) = matchedCellEnergy[1][19];
      decorator_CellEnergy_TileExt2_200(*track) = matchedCellEnergy[0][20];
      decorator_CellEnergy_TileExt2_100(*track) = matchedCellEnergy[1][20];

      decorator_EM_CellEnergy_0_200(*track) = totalEMCellEnergy[0];
      decorator_EM_CellEnergy_0_100(*track) = totalEMCellEnergy[1];
      decorator_HAD_CellEnergy_0_200(*track) = totalHADCellEnergy[0];
      decorator_HAD_CellEnergy_0_100(*track) = totalHADCellEnergy[1];
      decorator_Total_CellEnergy_0_200(*track) = totalCellEnergy[0];
      decorator_Total_CellEnergy_0_100(*track) = totalCellEnergy[1];

      decorator_ClusterEnergy_Energy (*track) = ClusterEnergy_Energy;
      decorator_ClusterEnergy_Eta (*track) = ClusterEnergy_Eta;
      decorator_ClusterEnergy_Phi (*track) = ClusterEnergy_Phi;
      decorator_ClusterEnergy_dRToTrack (*track) = ClusterEnergy_dRToTrack;
      decorator_ClusterEnergy_emProbability (*track) = ClusterEnergy_emProbability;
      decorator_ClusterEnergy_firstEnergyDensity (*track) = ClusterEnergy_firstEnergyDensity;
      decorator_ClusterEnergy_lambdaCenter (*track) = ClusterEnergy_lambdaCenter;
      decorator_ClusterEnergy_maxEnergyLayer (*track) = ClusterEnergy_maxEnergyLayer;

      decorator_CellEnergy_Energy (*track) = CellEnergy_Energy;
      decorator_CellEnergy_Eta (*track) = CellEnergy_Eta;
      decorator_CellEnergy_Phi (*track) = CellEnergy_Phi;
      decorator_CellEnergy_dRToTrack (*track) = CellEnergy_dRToTrack;
      decorator_CellEnergy_firstEnergyDensity (*track) = CellEnergy_firstEnergyDensity;
      decorator_CellEnergy_emProbability (*track) = CellEnergy_emProbability;
      decorator_CellEnergy_lambdaCenter (*track) = CellEnergy_lambdaCenter;
      decorator_CellEnergy_maxEnergyLayer (*track) = CellEnergy_maxEnergyLayer;

      decorator_ClusterEnergyLCW_Energy (*track) = ClusterEnergyLCW_Energy;
      decorator_ClusterEnergyLCW_Eta (*track) = ClusterEnergyLCW_Eta;
      decorator_ClusterEnergyLCW_Phi (*track) = ClusterEnergyLCW_Phi;
      decorator_ClusterEnergyLCW_dRToTrack (*track) = ClusterEnergyLCW_dRToTrack;
      decorator_ClusterEnergyLCW_lambdaCenter (*track) = ClusterEnergyLCW_lambdaCenter;
      decorator_ClusterEnergyLCW_emProbability (*track) = ClusterEnergyLCW_emProbability;
      decorator_ClusterEnergyLCW_firstEnergyDensity (*track) = ClusterEnergyLCW_firstEnergyDensity;
      decorator_ClusterEnergyLCW_maxEnergyLayer (*track) = ClusterEnergyLCW_maxEnergyLayer;

      if (m_doCutflow) {
        m_cutflow_trk -> Fill(m_cutflow_trk_pass_all, 1);
        evt_pass_all = true;
        ntrks_pass_all++;
      }
    } // loop trackContainer

    if (m_doCutflow) {
      m_ntrks_per_event_all -> Fill(ntrks_all, 1);
      m_ntrks_per_event_pass_all -> Fill(ntrks_pass_all, 1);
      if (evt_pass_all) m_cutflow_evt -> Fill(m_cutflow_evt_pass_all, 1);

      // Fill TTree with basic event info
      const xAOD::EventInfo* eventInfo(nullptr);
      CHECK( evtStore()->retrieve(eventInfo, m_eventInfoContainerName) );

      const_cast<int&>(m_runNumber)    = eventInfo->runNumber();
      const_cast<int&>(m_eventNumber)  = eventInfo->eventNumber();
      const_cast<int&>(m_lumiBlock)    = eventInfo->lumiBlock();
      const_cast<int&>(m_nTrks)        = ntrks_all;
      const_cast<int&>(m_nTrks_pass)   = ntrks_pass_all;

      m_tree->Fill();
    }

    return StatusCode::SUCCESS;
  }
} // Derivation Framework

