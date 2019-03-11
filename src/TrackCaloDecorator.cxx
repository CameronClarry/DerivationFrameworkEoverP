#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"
#include "CaloSimEvent/CaloCalibrationHitContainer.h"  
#include "MCTruthClassifier/IMCTruthClassifier.h"
#include "MCTruthClassifier/MCTruthClassifierDefs.h"
#include "CaloUtils/CaloClusterSignalState.h"

// tracks
#include "TrkTrack/Track.h"
#include "AthContainers/ConstDataVector.h"
#include "TrkParameters/TrackParameters.h"
#include "TrkExInterfaces/IExtrapolator.h"
#include "xAODTruth/TruthParticleContainer.h"
#include "xAODTracking/TrackParticleContainer.h"
#include <xAODTracking/VertexContainer.h>
#include <xAODTracking/Vertex.h>

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

#include <map>

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
    m_truthClassifier("MCTruthClassifier/MCTruthClassifier"),
    m_tileTBID(0),
    m_doCutflow{false}{
      declareInterface<DerivationFramework::IAugmentationTool>(this);
      declareProperty("DecorationPrefix", m_sgName);
      declareProperty("EventContainer", m_eventInfoContainerName);
      declareProperty("TrackContainer", m_trackContainerName);
      declareProperty("MCTruthClassifier", m_truthClassifier);
      declareProperty("CaloClusterContainer", m_caloClusterContainerName);
      declareProperty("Extrapolator", m_extrapolator);
      declareProperty("TheTrackExtrapolatorTool", m_theTrackExtrapolatorTool);
      declareProperty("DoCutflow", m_doCutflow);


    m_tileActiveHitCnt   = "TileCalibHitActiveCell";
    m_tileInactiveHitCnt = "TileCalibHitInactiveCell";
    m_tileDMHitCnt       = "TileCalibHitDeadMaterial";
    m_larActHitCnt   = "LArCalibrationHitActive";
    m_larInactHitCnt = "LArCalibrationHitInactive";
    m_larDMHitCnt    = "LArCalibrationHitDeadMaterial";

    }

  StatusCode TrackCaloDecorator::initialize()
  {
    if (m_sgName=="") {
      ATH_MSG_WARNING("No decoration prefix name provided for the output of TrackCaloDecorator!");
    }

    //OK initialize all of the radii that we want to use for track-to-cluster matching
    m_stringToCut["025"] = 0.025;
    m_stringToCut["050"] = 0.050;
    m_stringToCut["075"] = 0.075;
    m_stringToCut["100"] = 0.100;
    m_stringToCut["125"] = 0.125;
    m_stringToCut["150"] = 0.150;
    m_stringToCut["175"] = 0.175;
    m_stringToCut["125"] = 0.125;
    m_stringToCut["150"] = 0.150;
    m_stringToCut["175"] = 0.175;
    m_stringToCut["200"] = 0.200;
    m_stringToCut["225"] = 0.225;
    m_stringToCut["250"] = 0.250;
    m_stringToCut["275"] = 0.275;
    m_stringToCut["300"] = 0.300;

    //Get a list of all of the sampling numbers for the calorimeter
    ATH_MSG_INFO("Summing energy deposit info for layers: ");
    for (unsigned int i =0; i < CaloSampling::getNumberOfSamplings(); i++){
        m_caloSamplingNumbers.push_back((CaloSampling::CaloSample)(i));
        ATH_MSG_INFO(CaloSampling::getSamplingName(i));
    }

    //Get a list of strings for each of the cuts:
    ATH_MSG_INFO("Summing energy deposits at the following radii: ");
    for(std::map<std::string,float>::iterator it = m_stringToCut.begin(); it != m_stringToCut.end(); ++it) {
          m_cutNames.push_back(it->first);
          ATH_MSG_INFO(it->first);
    }

    //For each of the dR cuts and m_caloSamplingNumbers, create a decoration for the tracks
    ATH_MSG_INFO("Preparing Energy Deposit Decorators");
    for(std::string cutName : m_cutNames){
        ATH_MSG_INFO(cutName);
        //Create decorators for the total energy summed at different scales
        for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            const std::string caloSamplingName = CaloSampling::getSamplingName(caloSamplingNumber);
            SG::AuxElement::Decorator< float > cellDecorator(m_sgName + "_CellEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterDecorator(m_sgName + "_ClusterEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > lcwClusterDecorator(m_sgName + "_LCWClusterEnergy_" + caloSamplingName + "_" + cutName);
            m_cutToCaloSamplingNumberToDecorator_CellEnergy[cutName].insert(std::make_pair(caloSamplingNumber, cellDecorator));
            m_cutToCaloSamplingNumberToDecorator_ClusterEnergy[cutName].insert(std::make_pair(caloSamplingNumber,clusterDecorator));
            m_cutToCaloSamplingNumberToDecorator_LCWClusterEnergy[cutName].insert(std::make_pair(caloSamplingNumber,lcwClusterDecorator));
        }
    }

    //Create the decorators for the extrapolated track coordinates
    ATH_MSG_INFO("Perparing Decorators for the extrapolated track coordinates");
    for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
        const std::string caloSamplingName = CaloSampling::getSamplingName(caloSamplingNumber);
        ATH_MSG_INFO(caloSamplingName);
        m_caloSamplingNumberToDecorator_extrapolTrackEta.insert(std::make_pair(caloSamplingNumber, SG::AuxElement::Decorator< float >(m_sgName + "_trkEta_" + caloSamplingName)));
        m_caloSamplingNumberToDecorator_extrapolTrackPhi.insert(std::make_pair(caloSamplingNumber, SG::AuxElement::Decorator< float >(m_sgName + "_trkPhi_" + caloSamplingName)));
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


    // Retrieve track container, Cluster container and Cell container
    const xAOD::TrackParticleContainer* trackContainer = 0;
    CHECK(evtStore()->retrieve(trackContainer, m_trackContainerName));

    const xAOD::CaloClusterContainer* clusterContainer = 0; //xAOD object used to create decorations.
    CHECK(evtStore()->retrieve(clusterContainer, m_caloClusterContainerName));

    const CaloCellContainer *caloCellContainer = 0; //ESD object used to create decorations
    CHECK(evtStore()->retrieve(caloCellContainer, "AllCalo"));

    const xAOD::TruthParticleContainer* truthParticles = 0;
    CHECK(evtStore()->retrieve(truthParticles, "TruthParticles"));

    const CaloClusterCellLinkContainer* cclptr=0;
    if (evtStore()->contains<CaloClusterCellLinkContainer>(m_caloClusterContainerName+"_links")) {
      CHECK(evtStore()->retrieve(cclptr,m_caloClusterContainerName+"_links"));
      ATH_MSG_INFO("Found corresponding cell-link container with size " << cclptr->size());
    }
    else {ATH_MSG_INFO("Did not find corresponding cell-link container");}

    //Create the decorators for the eta, phi, energy, etc of clusters
    //We'll cut these at dR = 0.3
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Energy (m_sgName + "_ClusterEnergy_Energy");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Eta (m_sgName + "_ClusterEnergy_Eta");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_Phi (m_sgName + "_ClusterEnergy_Phi");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_dRToTrack (m_sgName + "_ClusterEnergy_dRToTrack");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_lambdaCenter (m_sgName + "_ClusterEnergy_lambdaCenter");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_emProbability (m_sgName + "_ClusterEnergy_emProbability");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergy_maxEnergyLayer (m_sgName + "_ClusterEnergy_maxEnergyLayer");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergy_IDNumber (m_sgName + "_ClusterEnergy_IDNumber");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_firstEnergyDensity (m_sgName + "_ClusterEnergy_firstEnergyDensity");

   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Energy (m_sgName + "_ClusterEnergyLCW_Energy");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Eta (m_sgName + "_ClusterEnergyLCW_Eta");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Phi (m_sgName + "_ClusterEnergyLCW_Phi");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_dRToTrack (m_sgName + "_ClusterEnergyLCW_dRToTrack");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_lambdaCenter (m_sgName + "_ClusterEnergyLCW_lambdaCenter");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_emProbability (m_sgName + "_ClusterEnergyLCW_emProbability");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergyLCW_maxEnergyLayer (m_sgName + "_ClusterEnergyLCW_maxEnergyLayer");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergyLCW_IDNumber (m_sgName + "_ClusterEnergyLCW_IDNumber");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_firstEnergyDensity (m_sgName + "_ClusterEnergyLCW_firstEnergyDensity");

   SG::AuxElement::Decorator<int> decorator_extrapolation (m_sgName + "_extrapolation");


    // Calibration hit containers
    const CaloCalibrationHitContainer* tile_actHitCnt = 0;
    const CaloCalibrationHitContainer* tile_inactHitCnt = 0;
    const CaloCalibrationHitContainer* tile_dmHitCnt = 0;
    const CaloCalibrationHitContainer* lar_actHitCnt = 0; 
    const CaloCalibrationHitContainer* lar_inactHitCnt = 0;
    const CaloCalibrationHitContainer* lar_dmHitCnt = 0;
    
    //retrieving input Calibhit containers
    bool hasCalibrationHits = true;
    if (!evtStore()->retrieve(tile_actHitCnt, m_tileActiveHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (!evtStore()->retrieve(tile_inactHitCnt, m_tileInactiveHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (!evtStore()->retrieve(tile_dmHitCnt,    m_tileDMHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (!evtStore()->retrieve(lar_actHitCnt,    m_larActHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (!evtStore()->retrieve(lar_inactHitCnt,  m_larInactHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (!evtStore()->retrieve(lar_dmHitCnt,     m_larDMHitCnt).isSuccess()) {
          hasCalibrationHits = false;
    }
    if (hasCalibrationHits) ATH_MSG_DEBUG("CaloCalibrationHitContainers retrieved successfuly" );
    else ATH_MSG_DEBUG("Could not retrieve CaloCalibrationHitContainers" );

    //Get the primary vertex
    const xAOD::VertexContainer *vtxs(nullptr);
    CHECK(evtStore()->retrieve(vtxs, "PrimaryVertices"));
    const xAOD::Vertex *primaryVertex(nullptr);
    for( auto vtx_itr : *vtxs )
    {
      if(vtx_itr->vertexType() != xAOD::VxType::VertexType::PriVtx) { primaryVertex = vtx_itr;}
    }

    bool evt_pass_all = false;
    int ntrks_all = 0;
    int ntrks_pass_all = 0;
    if (m_doCutflow) {
      m_cutflow_evt -> Fill(m_cutflow_evt_all, 1);
    }

    std::pair<unsigned int, unsigned int> res;
    MCTruthPartClassifier::ParticleDef partDef;

    for (const auto& track : *trackContainer) {
      //Create a calo calibration hit container for this matched particle
      //Create empty calocalibration hits containers
      if (m_doCutflow) {
        ntrks_all++;
        m_cutflow_trk -> Fill(m_cutflow_trk_all, 1);
      }

      res = m_truthClassifier->particleTruthClassifier(track);
      const xAOD::TruthParticle_v1* thePart = m_truthClassifier->getGenPart();
      bool hasTruthPart = (thePart != NULL);
      unsigned int particle_barcode = 0;
      if (hasTruthPart) {particle_barcode = thePart->barcode();}
      else {particle_barcode = 0;}

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
      decorator_ClusterEnergy_IDNumber (*track) = std::vector<int>();

      decorator_ClusterEnergyLCW_Energy (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Eta (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Phi (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_emProbability (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_firstEnergyDensity (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_dRToTrack (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_lambdaCenter (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_maxEnergyLayer (*track) = std::vector<int>();
      decorator_ClusterEnergyLCW_IDNumber (*track) = std::vector<int>();

      for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
          m_caloSamplingNumberToDecorator_extrapolTrackEta.at(caloSamplingNumber)(*track) = -999999999;
          m_caloSamplingNumberToDecorator_extrapolTrackPhi.at(caloSamplingNumber)(*track) = -999999999;
      }

      for (std::string cutName : m_cutNames){
          for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
              (m_cutToCaloSamplingNumberToDecorator_ClusterEnergy.at(cutName).at(caloSamplingNumber))(*track) = -999999999;
              (m_cutToCaloSamplingNumberToDecorator_LCWClusterEnergy.at(cutName).at(caloSamplingNumber))(*track) = -999999999;
              (m_cutToCaloSamplingNumberToDecorator_CellEnergy.at(cutName).at(caloSamplingNumber))(*track) = -999999999;
          }
      }

      /*a map to store the track parameters associated with the different layers of the calorimeter system */
      std::map<CaloSampling::CaloSample, const Trk::TrackParameters*> parametersMap;

      /*get the CaloExtension object*/
      const Trk::CaloExtension* extension = 0;

      if (m_theTrackExtrapolatorTool->caloExtension(*track, extension, false)) {

        /*extract the CurvilinearParameters per each layer-track intersection*/
        const std::vector<const Trk::CurvilinearParameters*>& clParametersVector = extension->caloLayerIntersections();

        for (auto clParameter : clParametersVector) {

          unsigned int parametersIdentifier = clParameter->cIdentifier();
          CaloSampling::CaloSample intLayer;

          if (!m_trackParametersIdHelper->isValid(parametersIdentifier)) {
            std::cout << "Invalid Track Identifier"<< std::endl;
            intLayer = CaloSampling::CaloSample::Unknown;
          } else {
            intLayer = (CaloSampling::CaloSample)(m_trackParametersIdHelper->caloSample(parametersIdentifier));
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

      //Decorate the tracks with their extrapolated coordinates
      for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
          if (parametersMap[caloSamplingNumber]){
              (m_caloSamplingNumberToDecorator_extrapolTrackPhi.at(caloSamplingNumber))(*track) = parametersMap[caloSamplingNumber]->position().phi();
              (m_caloSamplingNumberToDecorator_extrapolTrackEta.at(caloSamplingNumber))(*track) = parametersMap[caloSamplingNumber]->position().eta();
          }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_extrapolation, 1);

      /*Decorate track with extended eta and phi coordinates at intersection layers*/

      /*Track-cluster matching*/
      //Approach: find the most energetic layer of a cluster. Record the eta and phi coordinates of the extrapolated track at this layer//
      //Perform the matching between these track eta and phi coordinates and the energy-weighted (bary)centre eta and phi of the cluster//

      //create a const data vector for each dR cut
      std::map<std::string, ConstDataVector<xAOD::CaloClusterContainer> > matchedClusterMap;
      for (std::string cutName : m_cutNames){
          matchedClusterMap[cutName] = ConstDataVector<xAOD::CaloClusterContainer>(SG::VIEW_ELEMENTS);
      }

      std::vector<float> ClusterEnergy_Energy;
      std::vector<float> ClusterEnergy_Eta;
      std::vector<float> ClusterEnergy_Phi;
      std::vector<float> ClusterEnergy_dRToTrack;
      std::vector<float> ClusterEnergy_lambdaCenter;
      std::vector<float> ClusterEnergy_firstEnergyDensity;
      std::vector<float> ClusterEnergy_emProbability;
      std::vector<int> ClusterEnergy_IDNumber;
      std::vector<int> ClusterEnergy_maxEnergyLayer;

      std::vector<float> ClusterEnergyLCW_Energy;
      std::vector<float> ClusterEnergyLCW_Eta;
      std::vector<float> ClusterEnergyLCW_Phi;
      std::vector<float> ClusterEnergyLCW_dRToTrack;
      std::vector<float> ClusterEnergyLCW_lambdaCenter;
      std::vector<float> ClusterEnergyLCW_firstEnergyDensity;
      std::vector<float> ClusterEnergyLCW_emProbability;
      std::vector<int> ClusterEnergyLCW_IDNumber;
      std::vector<int> ClusterEnergyLCW_maxEnergyLayer;

      int clusterID = 0;
      for (const auto& cluster : *clusterContainer) {
        clusterID += 1;

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

        //Loop through the different dR Cuts, and push to the matched cluster container
        for (std::string cutName : m_cutNames){
            float cut = m_stringToCut.at(cutName);
            if (deltaR < cut) matchedClusterMap.at(cutName).push_back(cluster);
        }

        if(deltaR < 0.3){
          //push back the vector-like quantities that we want
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
          ClusterEnergy_IDNumber.push_back(clusterID);
          ClusterEnergy_firstEnergyDensity.push_back(first_energy_density);
          
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 501, lambda_center)) {ATH_MSG_WARNING("Couldn't retrieve the cluster lambda center");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 900, em_probability)) {ATH_MSG_WARNING("Couldn't rertieve the EM Probability");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 804, first_energy_density)) {ATH_MSG_WARNING("Couldn't rertieve the first energy density moment");}
          ClusterEnergyLCW_Energy.push_back(cluster->e());   //LCW Energy
          ClusterEnergyLCW_Eta.push_back(cluster->calEta()); // Eta and phi at LCW Scale
          ClusterEnergyLCW_Phi.push_back(cluster->calPhi()); // Eta and phi at LCW Scale
          ClusterEnergyLCW_dRToTrack.push_back(deltaR);
          ClusterEnergyLCW_lambdaCenter.push_back(lambda_center);
          ClusterEnergyLCW_maxEnergyLayer.push_back(mostEnergeticLayer);
          ClusterEnergyLCW_emProbability.push_back(em_probability);
          ClusterEnergyLCW_IDNumber.push_back(clusterID);
          ClusterEnergyLCW_firstEnergyDensity.push_back(first_energy_density);
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_cluster_matching, 1);

      /*Track-cell matching*/
      //Approach: loop over cell container, getting the eta and phi coordinates of each cell for each layer.//
      //Perform a match between the cell and the track eta and phi coordinates in the cell's sampling layer.//
      //
      std::map<std::string, ConstDataVector<CaloCellContainer> > matchedCellMap;
      for (std::string cutName :  m_cutNames){
          matchedCellMap[cutName] = ConstDataVector<CaloCellContainer>(SG::VIEW_ELEMENTS);
      }

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
        
        for (std::string cutName : m_cutNames){
            float cut = m_stringToCut.at(cutName);
            if (deltaR < cut) matchedCellMap.at(cutName).push_back(cell);
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_cell_matching, 1);

      for (std::string cutName : m_cutNames){

        //create std::maps of std::string CaloRegion -> float Energy Sum
        std::map<CaloSampling::CaloSample, float> caloSamplingNumberToEnergySum_EMScale;
        std::map<CaloSampling::CaloSample, float> caloSamplingNumberToEnergySum_LCWScale;

        for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            caloSamplingNumberToEnergySum_EMScale[caloSamplingNumber] = 0.0;
            caloSamplingNumberToEnergySum_LCWScale[caloSamplingNumber] = 0.0;
        }

        ConstDataVector<xAOD::CaloClusterContainer>::iterator firstMatchedClus = matchedClusterMap.at(cutName).begin();
        ConstDataVector<xAOD::CaloClusterContainer>::iterator lastMatchedClus = matchedClusterMap.at(cutName).end();

        /*Loop over matched clusters for a given cone dimension*/
        for (; firstMatchedClus != lastMatchedClus; ++firstMatchedClus) {

          const xAOD::CaloCluster* cl = *firstMatchedClus;
          float energy_EM = -999999999;
          float energy_LCW = -999999999;

          energy_EM = cl->rawE();
          energy_LCW = cl->calE();

          if(energy_EM == -999999999 || energy_LCW == -999999999) continue;

          double cluster_weight = energy_LCW/energy_EM;

          for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            caloSamplingNumberToEnergySum_EMScale[caloSamplingNumber] += cl->eSample(caloSamplingNumber);
            caloSamplingNumberToEnergySum_LCWScale[caloSamplingNumber] += cluster_weight*(cl->eSample(caloSamplingNumber));
          }

          //The map is hit type -> energy type (EM,nonEM,Invisible,Escaped) -> unsigned int CalorimeterLayer -> energy sum
          std::map<std::string, std::map< std::string, std::map<CaloSampling::CaloSample, float> > > hitsMap;
          std::map<std::string, std::map< std::string, std::map<int, std::map<CaloSampling::CaloSample, float> > > > bkgHitsMap;
          hitsMap["LarAct"] = getHitsSum(lar_actHitCnt, cl, particle_barcode);
          hitsMap["LarInact"] = getHitsSum(lar_inactHitCnt, cl, particle_barcode);
          hitsMap["TileAct"] = getHitsSum(tile_actHitCnt, cl, particle_barcode);
          hitsMap["TileInact"]= getHitsSum(tile_inactHitCnt, cl, particle_barcode);

          bkgHitsMap["LarAct"] = getHitsSumAllBackground(lar_actHitCnt ,cl, particle_barcode, truthParticles);
          bkgHitsMap["LarInact"] = getHitsSumAllBackground(lar_inactHitCnt ,cl, particle_barcode, truthParticles);
          bkgHitsMap["TileAct"] = getHitsSumAllBackground(tile_actHitCnt ,cl, particle_barcode, truthParticles);
          bkgHitsMap["TileInact"] = getHitsSumAllBackground(tile_inactHitCnt ,cl, particle_barcode, truthParticles);
        }
        //Loop over all of the calorimeter regions and decorate the tracks
        for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            (m_cutToCaloSamplingNumberToDecorator_ClusterEnergy.at(cutName).at(caloSamplingNumber))(*track) = caloSamplingNumberToEnergySum_EMScale.at(caloSamplingNumber);
            (m_cutToCaloSamplingNumberToDecorator_LCWClusterEnergy.at(cutName).at(caloSamplingNumber))(*track) = caloSamplingNumberToEnergySum_LCWScale.at(caloSamplingNumber);
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_loop_matched_clusters, 1);

      for (std::string cutName : m_cutNames){
        ConstDataVector<CaloCellContainer>::iterator firstMatchedCell = matchedCellMap.at(cutName).begin();
        ConstDataVector<CaloCellContainer>::iterator lastMatchedCell = matchedCellMap.at(cutName).end();

        std::map<CaloSampling::CaloSample, float> caloSamplingNumberToEnergySum_CellEnergy;
        for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            caloSamplingNumberToEnergySum_CellEnergy[caloSamplingNumber] = 0.0;
        }
        for (; firstMatchedCell != lastMatchedCell; ++firstMatchedCell) {
          if (!(*firstMatchedCell)->caloDDE()) continue;
          CaloCell_ID::CaloSample cellLayer = (*firstMatchedCell)->caloDDE()->getSampling();
          caloSamplingNumberToEnergySum_CellEnergy.at((CaloSampling::CaloSample)(cellLayer)) += (*firstMatchedCell)->energy();
        }
        //Decorate the tracks with the energy deposits in the correct layers
        for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
            (m_cutToCaloSamplingNumberToDecorator_CellEnergy.at(cutName).at(caloSamplingNumber))(*track) = caloSamplingNumberToEnergySum_CellEnergy.at(caloSamplingNumber);
        }
      }

      if (m_doCutflow) m_cutflow_trk -> Fill(m_cutflow_trk_pass_loop_matched_cells, 1);


      //Decorate the tracks with the vector-like quantities
      decorator_ClusterEnergy_Energy (*track) = ClusterEnergy_Energy;
      decorator_ClusterEnergy_Eta (*track) = ClusterEnergy_Eta;
      decorator_ClusterEnergy_Phi (*track) = ClusterEnergy_Phi;
      decorator_ClusterEnergy_dRToTrack (*track) = ClusterEnergy_dRToTrack;
      decorator_ClusterEnergy_emProbability (*track) = ClusterEnergy_emProbability;
      decorator_ClusterEnergy_firstEnergyDensity (*track) = ClusterEnergy_firstEnergyDensity;
      decorator_ClusterEnergy_lambdaCenter (*track) = ClusterEnergy_lambdaCenter;
      decorator_ClusterEnergy_maxEnergyLayer (*track) = ClusterEnergy_maxEnergyLayer;

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

   std::map< std::string, std::map<int, std::map<CaloSampling::CaloSample, float> > > TrackCaloDecorator::getHitsSumAllBackground(const CaloCalibrationHitContainer* hits, const xAOD::CaloCluster* cl,  unsigned int particle_barcode, const xAOD::TruthParticleContainer* truthParticles) const
  {
    //Sum all of the calibration hits in all of the layers, and return a map of calo layer to energy sum
    //Gather all of the information pertaining to the total energy deposited in the cells of this cluster
    
    //a std::map of energy type -> pdgID -> calocelllayer -> total energy deposited
    std::map<std::string, std::map<int, std::map<CaloSampling::CaloSample, float> > >hitsSum;

    hitsSum["EM"] = std::map<int, std::map<CaloSampling::CaloSample, float> >();
    hitsSum["NonEM"] = std::map<int, std::map<CaloSampling::CaloSample, float> >();
    hitsSum["Escaped"] = std::map<int, std::map<CaloSampling::CaloSample, float> >();
    hitsSum["Invisible"]= std::map< int, std::map<CaloSampling::CaloSample, float> >();

    //Loop though all of the truth particles in the event, and make sure that the std::map has an entry for it
    for(const auto& truthPart: *truthParticles){
        int pdgID = truthPart->pdgId();
        if (hitsSum["EM"].find( pdgID) == hitsSum["EM"].end()){
            hitsSum["EM"][pdgID] = std::map<CaloSampling::CaloSample, float>();
            hitsSum["NonEM"][pdgID] = std::map<CaloSampling::CaloSample, float>();
            hitsSum["Escaped"][pdgID] = std::map<CaloSampling::CaloSample, float>();
            hitsSum["Invisible"][pdgID] = std::map<CaloSampling::CaloSample, float>();
            for(CaloSampling::CaloSample m : m_caloSamplingNumbers) {
                hitsSum["EM"][pdgID][m] = 0.0;
                hitsSum["NonEM"][pdgID][m] = 0.0;
                hitsSum["Escaped"][pdgID][m] = 0.0;
                hitsSum["Invisible"][pdgID][m] = 0.0;
            }
        }
    }

    for(CaloSampling::CaloSample m : m_caloSamplingNumbers) {
        //Sum the energy from all particles 
        hitsSum["EM"][0][m] = 0.0;
        hitsSum["NonEM"][0][m] = 0.0;
        hitsSum["EscapedEM"][0][m] = 0.0;
        hitsSum["InvisibleEM"][0][m] = 0.0;
    }

    if (hits == NULL)
    {
        std::cout<<"hits container was null"<<std::endl;
        return hitsSum;
    }
    if (particle_barcode == 0){
        return hitsSum;
    }

    const CaloClusterCellLink* cellLinks=cl->getCellLinks();
    if ((cellLinks != NULL)){
       CaloCalibrationHitContainer::const_iterator it;
       const CaloCalibrationHit* hit = nullptr;

       for(it = hits->begin(); it!=hits->end(); it++) {
           hit = *it;

           //check if the hit is for the track particle. then it isn't a background. Don't sum for it.
           //check that we can find a truth particle for the hit. If we can, then sum the background for this pdgID. Else sum the energy into the background hits
           int pdgIDHit = 0;
           bool hitIsForTrackParticle = false;
           unsigned int hitID = hit->particleID();
           for(const auto& truthPart: *truthParticles){

               unsigned int truthPartBarcode = truthPart->barcode();

               if (hitID == truthPartBarcode){pdgIDHit = truthPart->pdgId(); break;}
               if (hitID == particle_barcode){hitIsForTrackParticle = true; break;}
           }

           //continue because the hit isn't background
           if (hitIsForTrackParticle) continue;


           CaloClusterCellLink::const_iterator lnk_it=cellLinks->begin();
           CaloClusterCellLink::const_iterator lnk_it_e=cellLinks->end();
           for (;lnk_it!=lnk_it_e;++lnk_it) {
               const CaloCell* cell=*lnk_it;
               CaloCell_ID::CaloSample cellLayer = cell->caloDDE()->getSampling();
               if (cell->ID() == hit->cellID()){
                   //std::cout<<"PDG ID = "<<pdgIDHit<< "   ID=" << std::hex << cell->ID() << std::dec << ", E=" << cell->e() << ", weight=" << lnk_it.weight() << std::endl;
                   hitsSum["EM"][pdgIDHit][cellLayer] += hit->energyEM();
                   hitsSum["NonEM"][pdgIDHit][cellLayer] += hit->energyNonEM();
                   hitsSum["Escaped"][pdgIDHit][cellLayer] += hit->energyEscaped();
                   hitsSum["Invisible"][pdgIDHit][cellLayer] += hit->energyInvisible();
               }
           }
       }
    }
    return hitsSum;
  }

  std::map<std::string, std::map<CaloSampling::CaloSample, float> > TrackCaloDecorator::getHitsSum(const CaloCalibrationHitContainer* hits,const  xAOD::CaloCluster* cl,  unsigned int particle_barcode) const{
    //Sum all of the calibration hits in all of the layers, and return a map of calo layer to energy sum
    //Gather all of the information pertaining to the total energy deposited in the cells of this cluster
    std::map<std::string, std::map<CaloSampling::CaloSample, float> > hitsSum;

    hitsSum["EM"] = std::map<CaloSampling::CaloSample, float>();
    hitsSum["NonEM"] = std::map<CaloSampling::CaloSample, float>();
    hitsSum["Escaped"] = std::map<CaloSampling::CaloSample, float>();
    hitsSum["Invisible"]= std::map<CaloSampling::CaloSample, float>();

    for(CaloSampling::CaloSample m : m_caloSamplingNumbers) {
        hitsSum["EM"][m] = 0.0;
        hitsSum["NonEM"][m] = 0.0;
        hitsSum["Escaped"][m] = 0.0;
        hitsSum["Invisible"][m] = 0.0;
    }

    if (hits == NULL)
    {
        std::cout<<"hits container was null"<<std::endl;
        return hitsSum;
    }

    if (particle_barcode == 0){
        return hitsSum;
    }

    const CaloClusterCellLink* cellLinks=cl->getCellLinks();
    if ((cellLinks != NULL)){
      CaloClusterCellLink::const_iterator lnk_it=cellLinks->begin();
      CaloClusterCellLink::const_iterator lnk_it_e=cellLinks->end();
      for (;lnk_it!=lnk_it_e;++lnk_it) {
          const CaloCell* cell=*lnk_it;
          CaloCell_ID::CaloSample cellLayer = cell->caloDDE()->getSampling();
          //std::cout<< "   ID=" << std::hex << cell->ID() << std::dec << ", E=" << cell->e() << ", weight=" << lnk_it.weight() << std::endl;
          CaloCalibrationHitContainer::const_iterator it;
          const CaloCalibrationHit* hit = nullptr;
          //Can we find a corresponding calibration hit for this cell?
          for(it = hits->begin(); it!=hits->end(); it++) {
              hit = *it;
              if ((cell->ID() == hit->cellID()) and (particle_barcode == hit->particleID())){
                  hitsSum["EM"][cellLayer] += hit->energyEM();
                  hitsSum["NonEM"][cellLayer] += hit->energyNonEM();
                  hitsSum["Escaped"][cellLayer] += hit->energyEscaped();
                  hitsSum["Invisible"][cellLayer] += hit->energyInvisible();
              }
          }
       }
    }
  return hitsSum;
  }
} // Derivation Framework
