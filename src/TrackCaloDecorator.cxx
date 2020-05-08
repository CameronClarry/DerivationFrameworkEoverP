#include "DerivationFrameworkEoverP/TrackCaloDecorator.h"
#include "CaloSimEvent/CaloCalibrationHitContainer.h"  
#include "MCTruthClassifier/IMCTruthClassifier.h"
#include "MCTruthClassifier/MCTruthClassifierDefs.h"
#include "CaloUtils/CaloClusterSignalState.h"
#include "Math/ProbFunc.h"

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

    //Get a list of all of the sampling numbers for the calorimeter
    ATH_MSG_INFO("Summing energy deposit info for layers: ");
    m_caloSamplingIndices = std::vector<unsigned int>();
    m_caloSamplingNumbers = std::vector<CaloSampling::CaloSample>();
    m_mapCaloSamplingToIndex = std::map<CaloSampling::CaloSample, unsigned int>();
    unsigned int count = 0;
    for (unsigned int i =0; i < m_nsamplings; i++){
        m_caloSamplingNumbers.push_back((CaloSampling::CaloSample)(i));
        m_caloSamplingIndices.push_back(count);
        m_mapCaloSamplingToIndex[(CaloSampling::CaloSample)(i)] = i;
        ATH_MSG_INFO(CaloSampling::getSamplingName(i));
        count += 1;
    }

    //Get a list of strings for each of the cuts:
    ATH_MSG_INFO("Summing energy deposits at the following radii: ");

    //Assign a number to each of the cuts
    unsigned int cutNumber = 0;
    for(std::map<std::string,float>::const_iterator it = m_stringToCut.begin(); it != m_stringToCut.end(); ++it) {
          m_cutNumbers.push_back(cutNumber);
          m_cutNumberToCut[cutNumber] = it->second;
          m_cutNumberToCutName[cutNumber] = it->first;
          m_cutNames.push_back(it->first);
          ATH_MSG_INFO(it->first);
          cutNumber += 1;
    }

    ////////////insert the decorators into the std maps
    m_cutToCaloSamplingIndexToDecorator_ClusterEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_LCWClusterEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_CellEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_LHED = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    //////////calib hits from truth matched particle/////////////
    //Active calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterNonEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterEscapedActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    //Inactive calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterNonEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterEscapedInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    
    //////////calib hits from background particles/////////////
    //Active calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    //Inactive calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    //////////calib hits from background particles/////////////
    //Active calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedActiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    //Inactive calibration hit energy
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );
    m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedInactiveCalibHitEnergy = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts );

    m_cutToCaloSamplingIndexToDecorator_WeigtedEnergyDensity = std::vector< std::vector<SG::AuxElement::Decorator< float > > >(m_ncuts);


    //For each of the dR cuts and m_caloSamplingNumbers, create a decoration for the tracks
    ATH_MSG_INFO("Preparing Energy Deposit Decorators");
    for(unsigned int cutNumber : m_cutNumbers){
        std::string cutName = m_cutNumberToCutName.at(cutNumber);
        ATH_MSG_INFO(cutName);
        //Create decorators for the total energy summed at different scales
        for (unsigned int sampling_index : m_caloSamplingIndices){
            CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
            const std::string caloSamplingName = CaloSampling::getSamplingName(caloSamplingNumber);
            SG::AuxElement::Decorator< float > cellDecorator(m_sgName + "_CellEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterDecorator(m_sgName + "_ClusterEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > lcwClusterDecorator(m_sgName + "_LCWClusterEnergy_" + caloSamplingName + "_" + cutName);

            //Active CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterEMActiveCalibHitDecorator(m_sgName + "_ClusterEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterNonEMActiveCalibHitDecorator(m_sgName + "_ClusterNonEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterEscapedActiveCalibHitDecorator(m_sgName + "_ClusterEscapedActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterInvisibleActiveCalibHitDecorator(m_sgName + "_ClusterInvisibleActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);

            //Inctive CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterEMInactiveCalibHitDecorator(m_sgName + "_ClusterEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterNonEMInactiveCalibHitDecorator(m_sgName + "_ClusterNonEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterEscapedInactiveCalibHitDecorator(m_sgName + "_ClusterEscapedInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterInvisibleInactiveCalibHitDecorator(m_sgName + "_ClusterInvisibleInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            
            //Photon Background Active CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundEMActiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundNonEMActiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundNonEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundEscapedActiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundEscapedActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundInvisibleActiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundInvisibleActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);

            //Photon Background Inactive CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundEMInactiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundNonEMInactiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundNonEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundEscapedInactiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundEscapedInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterPhotonBackgroundInvisibleInactiveCalibHitDecorator(m_sgName + "_ClusterPhotonBackgroundInvisibleInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);

            //Hadronic Background Active CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundEMActiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundNonEMActiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundNonEMActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundEscapedActiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundEscapedActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundInvisibleActiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundInvisibleActiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);

            //Hadronic Background Inactive CalibHit Energy Decorators
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundEMInactiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundNonEMInactiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundNonEMInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundEscapedInactiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundEscapedInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);
            SG::AuxElement::Decorator< float > clusterHadronicBackgroundInvisibleInactiveCalibHitDecorator(m_sgName + "_ClusterHadronicBackgroundInvisibleInactiveCalibHitEnergy_" + caloSamplingName + "_" + cutName);

            //Energy Density Decorators
            SG::AuxElement::Decorator< float > weigtedEnergyDensityDecorator(m_sgName + "_WeightedEnergyDensity_" + caloSamplingName + "_" + cutName);

            ////////////insert the decorators into the std maps
            m_cutToCaloSamplingIndexToDecorator_WeigtedEnergyDensity[cutNumber].push_back(weigtedEnergyDensityDecorator);

            m_cutToCaloSamplingIndexToDecorator_ClusterEnergy[cutNumber].push_back(clusterDecorator);
            m_cutToCaloSamplingIndexToDecorator_LCWClusterEnergy[cutNumber].push_back(lcwClusterDecorator);
            m_cutToCaloSamplingIndexToDecorator_CellEnergy[cutNumber].push_back(cellDecorator);

            //////////calib hits from truth matched particle/////////////
            //Active calibration hit energy
            m_cutToCaloSamplingIndexToDecorator_ClusterEMActiveCalibHitEnergy[cutNumber].push_back(clusterEMActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterNonEMActiveCalibHitEnergy[cutNumber].push_back(clusterNonEMActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleActiveCalibHitEnergy[cutNumber].push_back(clusterInvisibleActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterEscapedActiveCalibHitEnergy[cutNumber].push_back(clusterEscapedActiveCalibHitDecorator);

            //Inactive calibration hit energy
            m_cutToCaloSamplingIndexToDecorator_ClusterEMInactiveCalibHitEnergy[cutNumber].push_back(clusterEMInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterNonEMInactiveCalibHitEnergy[cutNumber].push_back(clusterNonEMInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleInactiveCalibHitEnergy[cutNumber].push_back(clusterInvisibleInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterEscapedInactiveCalibHitEnergy[cutNumber].push_back(clusterEscapedInactiveCalibHitDecorator);
            
            //////////calib hits from background particles/////////////
            //Active calibration hit energy
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMActiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundEMActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMActiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundNonEMActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleActiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundInvisibleActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedActiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundEscapedActiveCalibHitDecorator);

            //Inactive calibration hit energy
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMInactiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundEMInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMInactiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundNonEMInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleInactiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundInvisibleInactiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedInactiveCalibHitEnergy[cutNumber].push_back(clusterPhotonBackgroundEscapedInactiveCalibHitDecorator);

            //////////calib hits from background particles/////////////
            //Active calibration hit energy
            m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMActiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundEMActiveCalibHitDecorator);
            m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMActiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundNonEMActiveCalibHitDecorator);
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleActiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundInvisibleActiveCalibHitDecorator);
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedActiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundEscapedActiveCalibHitDecorator);

        //Inactive calibration hit energy
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMInactiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundEMInactiveCalibHitDecorator);
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMInactiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundNonEMInactiveCalibHitDecorator);
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleInactiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundInvisibleInactiveCalibHitDecorator);
        m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedInactiveCalibHitEnergy[cutNumber].push_back(clusterHadronicBackgroundEscapedInactiveCalibHitDecorator);
        }
    }

    m_caloSamplingIndexToDecorator_extrapolTrackEta.reserve(m_nsamplings);
    m_caloSamplingIndexToDecorator_extrapolTrackPhi.reserve(m_nsamplings);
    m_caloSamplingIndexToAccessor_extrapolTrackEta.reserve(m_nsamplings);
    m_caloSamplingIndexToAccessor_extrapolTrackPhi.reserve(m_nsamplings);
    //Create the decorators for the extrapolated track coordinates
    ATH_MSG_INFO("Perparing Decorators for the extrapolated track coordinates");
    for (unsigned int sampling_index : m_caloSamplingIndices){
        CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
        const std::string caloSamplingName = CaloSampling::getSamplingName(caloSamplingNumber);
        ATH_MSG_INFO(caloSamplingName);
        m_caloSamplingIndexToDecorator_extrapolTrackEta.push_back(SG::AuxElement::Decorator< float >(m_sgName + "_trkEta_" + caloSamplingName));
        m_caloSamplingIndexToDecorator_extrapolTrackPhi.push_back(SG::AuxElement::Decorator< float >(m_sgName + "_trkPhi_" + caloSamplingName));
        m_caloSamplingIndexToAccessor_extrapolTrackEta.push_back(SG::AuxElement::Accessor< float >(m_sgName + "_trkEta_" + caloSamplingName));
        m_caloSamplingIndexToAccessor_extrapolTrackPhi.push_back(SG::AuxElement::Accessor< float >(m_sgName + "_trkEta_" + caloSamplingName));
    }

    ATH_CHECK(m_extrapolator.retrieve());
    ATH_CHECK(m_theTrackExtrapolatorTool.retrieve());
    ATH_CHECK(m_surfaceHelper.retrieve());

    // Get the test beam identifier for the MBTS
    ATH_CHECK(detStore()->retrieve(m_tileTBID));

    // Save cutflow histograms
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
    bool hasTruthParticles = true;
    if (!evtStore()->retrieve(truthParticles, "TruthParticles").isSuccess()){hasTruthParticles = false;}

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

   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_deltaAlpha (m_sgName + "_ClusterEnergy_deltaAlpha");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_secondR (m_sgName + "_ClusterEnergy_secondR");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_secondLambda (m_sgName + "_ClusterEnergy_secondLambda");

   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_emProbability (m_sgName + "_ClusterEnergy_emProbability");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergy_maxEnergyLayer (m_sgName + "_ClusterEnergy_maxEnergyLayer");
   SG::AuxElement::Decorator< std::vector<int> > decorator_ClusterEnergy_IDNumber (m_sgName + "_ClusterEnergy_IDNumber");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergy_firstEnergyDensity (m_sgName + "_ClusterEnergy_firstEnergyDensity");

   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Energy (m_sgName + "_ClusterEnergyLCW_Energy");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Eta (m_sgName + "_ClusterEnergyLCW_Eta");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_Phi (m_sgName + "_ClusterEnergyLCW_Phi");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_dRToTrack (m_sgName + "_ClusterEnergyLCW_dRToTrack");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_lambdaCenter (m_sgName + "_ClusterEnergyLCW_lambdaCenter");

   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_deltaAlpha (m_sgName + "_ClusterEnergyLCW_deltaAlpha");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_secondR (m_sgName + "_ClusterEnergyLCW_secondR");
   SG::AuxElement::Decorator< std::vector<float> > decorator_ClusterEnergyLCW_secondLambda (m_sgName + "_ClusterEnergyLCW_secondLambda");

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

    std::pair<unsigned int, unsigned int> res;
    MCTruthPartClassifier::ParticleDef partDef;

    for (const auto& track : *trackContainer) {
      //Create a calo calibration hit container for this matched particle
      //Create empty calocalibration hits containers

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
      decorator_ClusterEnergy_deltaAlpha (*track) = std::vector<float>();
      decorator_ClusterEnergy_secondLambda (*track) = std::vector<float>();
      decorator_ClusterEnergy_secondR (*track) = std::vector<float>();
      decorator_ClusterEnergy_maxEnergyLayer (*track) = std::vector<int>();
      decorator_ClusterEnergy_IDNumber (*track) = std::vector<int>();

      decorator_ClusterEnergyLCW_Energy (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Eta (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_Phi (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_emProbability (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_firstEnergyDensity (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_dRToTrack (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_lambdaCenter (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_deltaAlpha (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_secondLambda (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_secondR (*track) = std::vector<float>();
      decorator_ClusterEnergyLCW_maxEnergyLayer (*track) = std::vector<int>();
      decorator_ClusterEnergyLCW_IDNumber (*track) = std::vector<int>();

      for (unsigned int sampling_index : m_caloSamplingIndices){
          CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
          m_caloSamplingIndexToDecorator_extrapolTrackEta.at(sampling_index)(*track) = -999999999;
          m_caloSamplingIndexToDecorator_extrapolTrackPhi.at(sampling_index)(*track) = -999999999;
      }

      //for (unsigned int cutNumber : m_cutNumbers){
      ///    for(CaloSampling::CaloSample caloSamplingNumber : m_caloSamplingNumbers){
      //        (m_cutToCaloSamplingIndexToDecorator_ClusterEnergy.at(cutNumber).at(sampling_index))(*track) = -999999999;
      //        (m_cutToCaloSamplingIndexToDecorator_LCWClusterEnergy.at(cutNumber).at(sampling_index))(*track) = -999999999;
      //        (m_cutToCaloSamplingIndexToDecorator_CellEnergy.at(cutNumber).at(sampling_index))(*track) = -999999999;
      //    }
      //}

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
      for (unsigned int sampling_index : m_caloSamplingIndices){
          CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
          if (parametersMap[caloSamplingNumber]){
              (m_caloSamplingIndexToDecorator_extrapolTrackPhi.at(sampling_index))(*track) = parametersMap[caloSamplingNumber]->position().phi();
              (m_caloSamplingIndexToDecorator_extrapolTrackEta.at(sampling_index))(*track) = parametersMap[caloSamplingNumber]->position().eta();
          }
      }


      /*Decorate track with extended eta and phi coordinates at intersection layers*/

      /*Track-cluster matching*/
      //Approach: find the most energetic layer of a cluster. Record the eta and phi coordinates of the extrapolated track at this layer//
      //Perform the matching between these track eta and phi coordinates and the energy-weighted (bary)centre eta and phi of the cluster//

      //create a const data vector for each dR cut
      std::vector<ConstDataVector<xAOD::CaloClusterContainer> > matchedClusterVector;
      matchedClusterVector.reserve(m_ncuts);
      for (unsigned int i = 0; i < m_ncuts; i++){
          matchedClusterVector.push_back(ConstDataVector<xAOD::CaloClusterContainer>(SG::VIEW_ELEMENTS));
      }


      std::vector<float> ClusterEnergy_Energy;
      std::vector<float> ClusterEnergy_Eta;
      std::vector<float> ClusterEnergy_Phi;
      std::vector<float> ClusterEnergy_dRToTrack;
      std::vector<float> ClusterEnergy_lambdaCenter;
      std::vector<float> ClusterEnergy_secondLambda;
      std::vector<float> ClusterEnergy_secondR;
      std::vector<float> ClusterEnergy_deltaAlpha;
      std::vector<float> ClusterEnergy_firstEnergyDensity;
      std::vector<float> ClusterEnergy_emProbability;
      std::vector<int> ClusterEnergy_IDNumber;
      std::vector<int> ClusterEnergy_maxEnergyLayer;

      std::vector<float> ClusterEnergyLCW_Energy;
      std::vector<float> ClusterEnergyLCW_Eta;
      std::vector<float> ClusterEnergyLCW_Phi;
      std::vector<float> ClusterEnergyLCW_dRToTrack;
      std::vector<float> ClusterEnergyLCW_lambdaCenter;
      std::vector<float> ClusterEnergyLCW_secondLambda;
      std::vector<float> ClusterEnergyLCW_secondR;
      std::vector<float> ClusterEnergyLCW_deltaAlpha;
      std::vector<float> ClusterEnergyLCW_firstEnergyDensity;
      std::vector<float> ClusterEnergyLCW_emProbability;
      std::vector<int> ClusterEnergyLCW_IDNumber;
      std::vector<int> ClusterEnergyLCW_maxEnergyLayer;

      int clusterID = 0;
      for (const auto& cluster : *clusterContainer) {
        clusterID += 1;

        /*Finding the most energetic layer of the cluster*/
        xAOD::CaloCluster::CaloSample mostEnergeticLayer = xAOD::CaloCluster::CaloSample::Unknown;
        std::pair<xAOD::CaloCluster::CaloSample, double> layer_energy = get_most_energetic_layer(cluster);
        mostEnergeticLayer = layer_energy.first;
        double maxLayerClusterEnergy = layer_energy.second;

        if(mostEnergeticLayer==xAOD::CaloCluster::CaloSample::Unknown) continue;

        //do track-cluster matching at EM-Scale
        double clEta = cluster->rawEta();
        double clPhi = cluster->rawPhi();

        if(clEta == -999 || clPhi == -999) continue;

        /*Matching between the track parameters in the most energetic layer and the cluster barycentre*/

        if(!parametersMap[mostEnergeticLayer]) continue;

        double trackEta = parametersMap[mostEnergeticLayer]->position().eta();
        double trackPhi = parametersMap[mostEnergeticLayer]->position().phi();
        double deltaR = TrackCaloDecorator::calc_angular_distance(trackEta, trackPhi, clEta, clPhi);

        if(deltaR < 0.3){
          //push back the vector-like quantities that we want
          double lambda_center;
          double em_probability;
          double first_energy_density;
          double second_lambda;
          double delta_alpha;
          double second_r;


          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 501, lambda_center)) {ATH_MSG_WARNING("Couldn't retrieve the cluster lambda center");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 900, em_probability)) {ATH_MSG_WARNING("Couldn't rertieve the EM Probability");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 804, first_energy_density)) {ATH_MSG_WARNING("Couldn't rertieve the first energy density moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 303, delta_alpha)) {ATH_MSG_WARNING("Couldn't rertieve the delta alpha moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 202, second_lambda)) {ATH_MSG_WARNING("Couldn't rertieve the second lambda moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 202, second_r)) {ATH_MSG_WARNING("Couldn't rertieve the second radial");}

          //we want to include the information about these clusters in the derivation output
          ClusterEnergy_Energy.push_back(cluster->rawE()); //Raw Energy
          ClusterEnergy_Eta.push_back(cluster->rawEta()); //Eta and phi based on EM Scale
          ClusterEnergy_Phi.push_back(cluster->rawPhi()); //Eta and phi based on EM Scale
          ClusterEnergy_dRToTrack.push_back(deltaR);

          ClusterEnergy_lambdaCenter.push_back(lambda_center);
          ClusterEnergy_secondLambda.push_back(second_lambda);
          ClusterEnergy_deltaAlpha.push_back(delta_alpha);
          ClusterEnergy_secondR.push_back(second_r);

          ClusterEnergy_maxEnergyLayer.push_back(mostEnergeticLayer);
          ClusterEnergy_emProbability.push_back(em_probability);
          ClusterEnergy_IDNumber.push_back(clusterID);
          ClusterEnergy_firstEnergyDensity.push_back(first_energy_density);
          
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 501, lambda_center)) {ATH_MSG_WARNING("Couldn't retrieve the cluster lambda center");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 900, em_probability)) {ATH_MSG_WARNING("Couldn't rertieve the EM Probability");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 804, first_energy_density)) {ATH_MSG_WARNING("Couldn't rertieve the first energy density moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 303, delta_alpha)) {ATH_MSG_WARNING("Couldn't rertieve the delta alpha moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 202, second_lambda)) {ATH_MSG_WARNING("Couldn't rertieve the second lambda moment");}
          if (!cluster->retrieveMoment((xAOD::CaloCluster_v1::MomentType) 202, second_r)) {ATH_MSG_WARNING("Couldn't rertieve the second radial");}
          ClusterEnergyLCW_Energy.push_back(cluster->e());   //LCW Energy
          ClusterEnergyLCW_Eta.push_back(cluster->calEta()); // Eta and phi at LCW Scale
          ClusterEnergyLCW_Phi.push_back(cluster->calPhi()); // Eta and phi at LCW Scale
          ClusterEnergyLCW_dRToTrack.push_back(deltaR);
          ClusterEnergyLCW_lambdaCenter.push_back(lambda_center);
          ClusterEnergyLCW_secondLambda.push_back(second_lambda);
          ClusterEnergyLCW_deltaAlpha.push_back(delta_alpha);
          ClusterEnergyLCW_secondR.push_back(second_r);
          ClusterEnergyLCW_maxEnergyLayer.push_back(mostEnergeticLayer);
          ClusterEnergyLCW_emProbability.push_back(em_probability);
          ClusterEnergyLCW_IDNumber.push_back(clusterID);
          ClusterEnergyLCW_firstEnergyDensity.push_back(first_energy_density);
        }
        //Loop through the different dR Cuts, and push to the matched cluster container
        for (unsigned int cutNumber: m_cutNumbers){
            float cut = m_cutNumberToCut.at(cutNumber);
            if (deltaR < cut) {
                matchedClusterVector.at(cutNumber).push_back(cluster);
                break;
            }
        }
      }

      //Decorate the tracks with the vector-like quantities
      decorator_ClusterEnergy_Energy (*track) = ClusterEnergy_Energy;
      decorator_ClusterEnergy_Eta (*track) = ClusterEnergy_Eta;
      decorator_ClusterEnergy_Phi (*track) = ClusterEnergy_Phi;
      decorator_ClusterEnergy_dRToTrack (*track) = ClusterEnergy_dRToTrack;
      decorator_ClusterEnergy_emProbability (*track) = ClusterEnergy_emProbability;
      decorator_ClusterEnergy_firstEnergyDensity (*track) = ClusterEnergy_firstEnergyDensity;
      decorator_ClusterEnergy_lambdaCenter (*track) = ClusterEnergy_lambdaCenter;
      decorator_ClusterEnergy_deltaAlpha (*track) = ClusterEnergy_deltaAlpha;
      decorator_ClusterEnergy_secondLambda (*track) = ClusterEnergy_secondLambda;
      decorator_ClusterEnergy_secondR (*track) = ClusterEnergy_secondR;
      decorator_ClusterEnergy_maxEnergyLayer (*track) = ClusterEnergy_maxEnergyLayer;

      decorator_ClusterEnergyLCW_Energy (*track) = ClusterEnergyLCW_Energy;
      decorator_ClusterEnergyLCW_Eta (*track) = ClusterEnergyLCW_Eta;
      decorator_ClusterEnergyLCW_Phi (*track) = ClusterEnergyLCW_Phi;
      decorator_ClusterEnergyLCW_dRToTrack (*track) = ClusterEnergyLCW_dRToTrack;
      decorator_ClusterEnergyLCW_lambdaCenter (*track) = ClusterEnergyLCW_lambdaCenter;
      decorator_ClusterEnergyLCW_deltaAlpha (*track) = ClusterEnergyLCW_deltaAlpha;
      decorator_ClusterEnergyLCW_secondLambda (*track) = ClusterEnergyLCW_secondLambda;
      decorator_ClusterEnergyLCW_secondR (*track) = ClusterEnergyLCW_secondR;
      decorator_ClusterEnergyLCW_emProbability (*track) = ClusterEnergyLCW_emProbability;
      decorator_ClusterEnergyLCW_firstEnergyDensity (*track) = ClusterEnergyLCW_firstEnergyDensity;
      decorator_ClusterEnergyLCW_maxEnergyLayer (*track) = ClusterEnergyLCW_maxEnergyLayer;


      /*Track-cell matching*/
      //Approach: loop over cell container, getting the eta and phi coordinates of each cell for each layer.//
      //Perform a match between the cell and the track eta and phi coordinates in the cell's sampling layer.//
      //
      std::vector<ConstDataVector<CaloCellContainer> > matchedCellVector;
      matchedCellVector.reserve(m_ncuts);
      for (unsigned int i = 0; i < m_ncuts; i++){
          matchedCellVector.push_back(ConstDataVector<CaloCellContainer>(SG::VIEW_ELEMENTS));
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

          for (unsigned int cutNumber: m_cutNumbers){
              float cut = m_cutNumberToCut.at(cutNumber);
              if (deltaR < cut) {
                  matchedCellVector.at(cutNumber).push_back(cell);
                  break;
              }
          }
      }

      std::vector<float> caloSamplingIndexToEnergySum_EMScale(m_nsamplings);
      std::vector<float> caloSamplingIndexToEnergySum_LCWScale(m_nsamplings);

      //EM == 0
      //NonEM == 1
      //Invisible == 2
      //Escaped == 3
      std::vector< std::vector<float> > energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit(4, std::vector<float>(m_nsamplings));
      std::vector< std::vector<float> > energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit(4, std::vector<float>(m_nsamplings));

      std::vector< std::vector<float> > photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit(4, std::vector<float>(m_nsamplings));
      std::vector< std::vector<float> > photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit(4, std::vector<float>(m_nsamplings));

      std::vector< std::vector<float> > hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit(4, std::vector<float>(m_nsamplings));
      std::vector< std::vector<float> > hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit(4, std::vector<float>(m_nsamplings));

      for (unsigned int sampling_index : m_caloSamplingIndices){
          caloSamplingIndexToEnergySum_EMScale[sampling_index] = 0.0;
          caloSamplingIndexToEnergySum_LCWScale[sampling_index] = 0.0;

          energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index] = 0.0;

          energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[0][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[1][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[2][sampling_index] = 0.0;
          energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[3][sampling_index] = 0.0;

          photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index] = 0.0;
          photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index] = 0.0;
          photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index] = 0.0;
          photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index] = 0.0;

          hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index] = 0.0;
          hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index] = 0.0;
          hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index] = 0.0;
          hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index] = 0.0;

      }

      std::vector<int> PhotonPDGID;
      PhotonPDGID.push_back(22);
      std::vector<int> EmptyVectorPDGID;

      std::map<xAOD::CaloCluster::CaloSample, float> density_sum_map;
      density_sum_map = TrackCaloDecorator::initialize_Empty_Sum_Map();

      for (unsigned int cutNumber: m_cutNumbers){
          std::string cutName = m_cutNumberToCutName.at(cutNumber);
          //create std::maps of std::string CaloRegion -> float Energy Sum
          ConstDataVector<xAOD::CaloClusterContainer>::iterator firstMatchedClus = matchedClusterVector.at(cutNumber).begin();
          ConstDataVector<xAOD::CaloClusterContainer>::iterator lastMatchedClus = matchedClusterVector.at(cutNumber).end();

          ConstDataVector<xAOD::CaloClusterContainer> matchedClusters = matchedClusterVector.at(cutNumber);
          std::map<xAOD::CaloCluster::CaloSample, float> this_density_map = TrackCaloDecorator::calc_LHED(matchedClusters, track);
          
          //add the maps together, and decorate the track for this sampling
          for (unsigned int sampling_index : m_caloSamplingIndices){
              CaloSampling::CaloSample sampling = m_caloSamplingNumbers.at(sampling_index);
              density_sum_map[sampling] += this_density_map[sampling];
              m_cutToCaloSamplingIndexToDecorator_LHED.at(cutNumber).at(sampling_index)(*track) = density_sum_map[sampling];
          }

          /*Loop over matched clusters for a given cone dimension*/
          for (; firstMatchedClus != lastMatchedClus; ++firstMatchedClus) {

              const xAOD::CaloCluster* cl = *firstMatchedClus;
              float energy_EM = -999999999;
              float energy_LCW = -999999999;

              energy_EM = cl->rawE();
              energy_LCW = cl->calE();
              double cluster_weight = energy_LCW/energy_EM;

              if(energy_EM == -999999999 || energy_LCW == -999999999) continue;

              for (unsigned int sampling_index : m_caloSamplingIndices){
                  CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
                  caloSamplingIndexToEnergySum_EMScale[sampling_index] += cl->eSample(caloSamplingNumber);
                  caloSamplingIndexToEnergySum_LCWScale[sampling_index] += cluster_weight*(cl->eSample(caloSamplingNumber));
              }

              getHitsSum(lar_actHitCnt, cl, particle_barcode, energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
              getHitsSum(lar_inactHitCnt, cl, particle_barcode, energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);
              getHitsSum(tile_actHitCnt, cl, particle_barcode, energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
              getHitsSum(tile_inactHitCnt, cl, particle_barcode, energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);


              if (hasCalibrationHits and hasTruthParticles){
                  getHitsSumAllBackground(lar_actHitCnt ,cl, particle_barcode, truthParticles, PhotonPDGID, EmptyVectorPDGID, photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
                  getHitsSumAllBackground(lar_inactHitCnt ,cl, particle_barcode, truthParticles, PhotonPDGID, EmptyVectorPDGID, photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);
                  getHitsSumAllBackground(tile_actHitCnt ,cl, particle_barcode, truthParticles, PhotonPDGID, EmptyVectorPDGID, photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
                  getHitsSumAllBackground(tile_inactHitCnt ,cl, particle_barcode, truthParticles, PhotonPDGID, EmptyVectorPDGID, photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);

                  getHitsSumAllBackground(lar_actHitCnt ,cl, particle_barcode, truthParticles, EmptyVectorPDGID, PhotonPDGID, hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
                  getHitsSumAllBackground(lar_inactHitCnt ,cl, particle_barcode, truthParticles, EmptyVectorPDGID, PhotonPDGID, hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);
                  getHitsSumAllBackground(tile_actHitCnt ,cl, particle_barcode, truthParticles, EmptyVectorPDGID, PhotonPDGID, hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit);
                  getHitsSumAllBackground(tile_inactHitCnt ,cl, particle_barcode, truthParticles, EmptyVectorPDGID, PhotonPDGID, hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit);
              }


          }
          for (unsigned int sampling_index : m_caloSamplingIndices){
              CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];

              //Decorate the tracks with the sum of the hits
              (m_cutToCaloSamplingIndexToDecorator_ClusterEnergy.at(cutNumber).at(sampling_index))(*track) = caloSamplingIndexToEnergySum_EMScale.at(sampling_index);
              (m_cutToCaloSamplingIndexToDecorator_LCWClusterEnergy.at(cutNumber).at(sampling_index))(*track) = caloSamplingIndexToEnergySum_LCWScale.at(sampling_index);

              if (hasCalibrationHits and hasTruthParticles){
                  (m_cutToCaloSamplingIndexToDecorator_ClusterEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterNonEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterEscapedActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index];

                  (m_cutToCaloSamplingIndexToDecorator_ClusterEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterNonEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterEscapedInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = energyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[3][sampling_index];

                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index];

                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = photonBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[3][sampling_index];

                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedActiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_ActiveCalibHit[3][sampling_index];

                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[0][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[1][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[2][sampling_index];
                  (m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedInactiveCalibHitEnergy.at(cutNumber).at(sampling_index))(*track) = hadronicBkgEnergyTypeToCaloSamplingIndexToEnergySum_InactiveCalibHit[3][sampling_index];
              }

          }//close loop over calo sampling numbers
          ATH_MSG_DEBUG("Done looping over clusters for cut " + cutName);
      }//close loop over cut names

      //sum energy deposits from cells
      std::vector<float> caloSamplingIndexToEnergySum_CellEnergy(m_nsamplings);
      for (unsigned int sampling_index : m_caloSamplingIndices){
          CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
          caloSamplingIndexToEnergySum_CellEnergy[sampling_index] = 0.0;
      }
      for (unsigned int cutNumber : m_cutNumbers){
          ConstDataVector<CaloCellContainer>::iterator firstMatchedCell = matchedCellVector.at(cutNumber).begin();
          ConstDataVector<CaloCellContainer>::iterator lastMatchedCell = matchedCellVector.at(cutNumber).end();
          for (; firstMatchedCell != lastMatchedCell; ++firstMatchedCell) {
              if (!(*firstMatchedCell)->caloDDE()) continue;
              CaloCell_ID::CaloSample cellLayer = (*firstMatchedCell)->caloDDE()->getSampling();
              caloSamplingIndexToEnergySum_CellEnergy.at(m_mapCaloSamplingToIndex.at(((CaloSampling::CaloSample)(cellLayer)))) += (*firstMatchedCell)->energy();
          }
          //Decorate the tracks with the energy deposits in the correct layers
          for (unsigned int sampling_index : m_caloSamplingIndices){
              (m_cutToCaloSamplingIndexToDecorator_CellEnergy.at(cutNumber).at(sampling_index))(*track) = caloSamplingIndexToEnergySum_CellEnergy.at(sampling_index);
          }
      }//close loop over cut names
    } // loop trackContainer
    return StatusCode::SUCCESS;
  }
  void TrackCaloDecorator::getHitsSum(const CaloCalibrationHitContainer* hits,const  xAOD::CaloCluster* cl,  unsigned int particle_barcode, std::vector< std::vector<float> >& hitsMap) const {
       //Sum all of the calibration hits in all of the layers, and return a map of calo layer to energy sum
       if (hits == NULL)
       {
           return;
       }

       if (particle_barcode == 0){
           return;
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
               //Can we find a corresponding calibration hit for this cell?
               for(it = hits->begin(); it!=hits->end(); it++) {
                   const CaloCalibrationHit* hit = *it;
                   if ((cell->ID() == hit->cellID()) and (particle_barcode == hit->particleID())){
                       unsigned int cell_layer_index = m_mapCaloSamplingToIndex.at(((CaloSampling::CaloSample)(cellLayer)));
                       hitsMap[0][cell_layer_index] += hit->energyEM();
                       hitsMap[1][cell_layer_index] += hit->energyNonEM();
                       hitsMap[2][cell_layer_index] += hit->energyInvisible();
                       hitsMap[3][cell_layer_index] += hit->energyEscaped();
                   }
               }
           }
       }
    }

    void TrackCaloDecorator::getHitsSumAllBackground(const CaloCalibrationHitContainer* hits, const xAOD::CaloCluster* cl,  unsigned int particle_barcode, const xAOD::TruthParticleContainer* truthParticles, std::vector<int> sumForThesePDGIDs, std::vector<int> skipThesePDGIDs,  std::vector< std::vector<float> >& hitsMap) const {
      //Sum all of the calibration hits in all of the layers, and return a map of calo layer to energy sum
      //Gather all of the information pertaining to the total energy deposited in the cells of this cluster
      if (particle_barcode == 0) return;

      if (hits == NULL)
      {
          std::cout<<"hits container was null"<<std::endl;
          return;
      }

      const CaloClusterCellLink* cellLinks=cl->getCellLinks();
      if ((cellLinks != NULL)){
          CaloCalibrationHitContainer::const_iterator it;
          const CaloCalibrationHit* hit = nullptr;

          for(it = hits->begin(); it!=hits->end(); it++) {
              hit = *it;

              //check if the hit is for the track particle. If it is from the track particle, it isn't background. Don't sum the energy deposits for it.
              unsigned int hitID = hit->particleID();
              if (hitID == particle_barcode){continue;}

              //check that the hit was in the cluster
              bool hitInCluster = false;
              CaloClusterCellLink::const_iterator lnk_it=cellLinks->begin();
              CaloClusterCellLink::const_iterator lnk_it_e=cellLinks->end();
              CaloCell_ID::CaloSample cellLayer;
              for (;lnk_it!=lnk_it_e;++lnk_it) {
                  const CaloCell* cell=*lnk_it;
                  if (cell->ID() == hit->cellID()){
                      hitInCluster=true;
                      cellLayer = cell->caloDDE()->getSampling();
                      break;
                  }
              }
              if (not hitInCluster){continue;}

              //loop through the truth particle container, and find the pdg of the truth particle that caused the hit
              int pdgIDHit = 0;
              for(const auto& truthPart: *truthParticles){
                  unsigned int truthPartBarcode = truthPart->barcode();
                  if (hitID == truthPartBarcode){pdgIDHit = truthPart->pdgId(); break;} 
              }
              if (pdgIDHit == 0){ATH_MSG_WARNING("Warning, couldn't find a truth particle for this hit");}

              //Check if this is a PDG ID that should be summed
              //bool shouldSum = true;
              if (sumForThesePDGIDs.size() != 0){
                  if (std::find(sumForThesePDGIDs.begin(), sumForThesePDGIDs.end(), pdgIDHit) == sumForThesePDGIDs.end()){
                      //shouldSum = false;
                      continue;
                  }
              }
              //Check if this is a PDG ID that should be skipped
              else if (skipThesePDGIDs.size() != 0){
                  if (std::find(skipThesePDGIDs.begin(), skipThesePDGIDs.end(), pdgIDHit) != skipThesePDGIDs.end()){
                      //shouldSum = false;
                      continue;
                  }
              }
              unsigned int cell_layer_index = m_mapCaloSamplingToIndex.at(((CaloSampling::CaloSample)(cellLayer)));

              //std::cout<<"PDG ID = "<<pdgIDHit<< "   ID=" << std::hex << cell->ID() << std::dec << ", E=" << cell->e() << ", weight=" << lnk_it.weight() << std::endl;
              hitsMap[0][cell_layer_index]+=hit->energyEM();
              hitsMap[1][cell_layer_index]+=hit->energyNonEM();
              hitsMap[2][cell_layer_index]+=hit->energyInvisible();
              hitsMap[3][cell_layer_index]+=hit->energyEscaped();
          }
     }
  }

  float TrackCaloDecorator::calc_angular_distance(float eta_obj1, float phi_obj1, float eta_obj2, float phi_obj2) const {
          float etaDiff = eta_obj1 - eta_obj2;
          float phiDiff = phi_obj1 - phi_obj2;
          if (phiDiff > TMath::Pi()) phiDiff = 2*TMath::Pi() - phiDiff;
          return std::sqrt((etaDiff*etaDiff) + (phiDiff*phiDiff));
  }

  std::map<xAOD::CaloCluster::CaloSample, float> TrackCaloDecorator::initialize_Empty_Sum_Map() const {
    std::map<CaloSampling::CaloSample, float> to_return;
        for (unsigned int sampling_index : m_caloSamplingIndices){
        CaloSampling::CaloSample caloSamplingNumber = m_caloSamplingNumbers[sampling_index];
        to_return[caloSamplingNumber] = 0.0;
     }
    return to_return;
  }

  std::map<xAOD::CaloCluster::CaloSample, float> TrackCaloDecorator::calc_LHED(ConstDataVector<xAOD::CaloClusterContainer> &clusters, const xAOD::TrackParticle* trk) const {

    std::map<CaloSampling::CaloSample, float> densities = TrackCaloDecorator::initialize_Empty_Sum_Map();
    //Go through the various extrapolated coordinates of the tracks
    //Lets loop through the clusters:
    for (const auto& cl : clusters) {
        std::pair<xAOD::CaloCluster::CaloSample, double> sampling_energy_pair = get_most_energetic_layer(cl);
        xAOD::CaloCluster::CaloSample most_energetic_layer = sampling_energy_pair.first;
        if (most_energetic_layer == xAOD::CaloCluster::CaloSample::Unknown) continue;
        if (not m_caloSamplingIndexToAccessor_extrapolTrackEta.at(most_energetic_layer).isAvailable(*trk) or 
            not m_caloSamplingIndexToAccessor_extrapolTrackPhi.at(most_energetic_layer).isAvailable(*trk)) continue;
        float extrapolEta = m_caloSamplingIndexToAccessor_extrapolTrackEta.at(most_energetic_layer)(*trk);
        float extrapolPhi = m_caloSamplingIndexToAccessor_extrapolTrackPhi.at(most_energetic_layer)(*trk);

        double clEta = cl->rawEta();
        double clPhi = cl->rawPhi();
        if(clEta == -999 || clPhi == -999) continue;

        //Lets calculate the LHED
        float LHED_scale = 0.035;// taken from https://arxiv.org/pdf/1703.10485.pdf
        //loop over the cells
        const CaloClusterCellLink* cellLinks=cl->getCellLinks();
        if ((cellLinks == NULL)){ continue;}

        CaloClusterCellLink::const_iterator lnk_it=cellLinks->begin();
        CaloClusterCellLink::const_iterator lnk_it_e=cellLinks->end();
        CaloCell_ID::CaloSample cellLayer;
        for (;lnk_it!=lnk_it_e;++lnk_it) {
             const CaloCell* cell=*lnk_it;
             const CaloDetDescrElement* dde = cell->caloDDE();
             cellLayer = dde->getSampling();
             float volume = dde->volume();
             float cell_dr = dde->dr();
             float cell_dphi = dde->dphi();
             float cell_eta = dde->eta_raw();
             float cell_phi = dde->phi_raw();
             float cell_track_dr = TrackCaloDecorator::calc_angular_distance(cell_eta, cell_phi, extrapolEta, extrapolPhi);
             float cluster_dr_up = cell_track_dr + (cell_dr / 2.0);
             float cluster_dr_down = cell_track_dr - (cell_dr / 2.0);
             double cdf_low = ROOT::Math::normal_cdf ( cluster_dr_down, LHED_scale,  0.0 );
             double cdf_high = ROOT::Math::normal_cdf ( cluster_dr_up , LHED_scale, 0.0 );
             double weight = (cdf_high - cdf_low) * cell_dphi;
             //OK Now lets calculate the cell energy density
             double density = cell->energy() * weight / volume; //density with weight applied
             std::cout<<"The density was "<<density<<std::endl;
             densities[CaloSampling::CaloSample(cellLayer)] += density;
        }
    }
    return densities;
  }

  std::pair<xAOD::CaloCluster::CaloSample, double> TrackCaloDecorator::get_most_energetic_layer(const xAOD::CaloCluster* cl) const {
    double maxLayerClusterEnergy = -999999999.0;
    xAOD::CaloCluster::CaloSample mostEnergeticLayer = xAOD::CaloCluster::CaloSample::Unknown;
    for (int i=0; i<xAOD::CaloCluster::CaloSample::TileExt2+1; i++) {
      xAOD::CaloCluster::CaloSample sampleLayer = (xAOD::CaloCluster::CaloSample)(i);
      double clusterLayerEnergy = cl->eSample(sampleLayer);
      if(clusterLayerEnergy > maxLayerClusterEnergy) {
        maxLayerClusterEnergy = clusterLayerEnergy;
        mostEnergeticLayer = sampleLayer;
      }
    }
    return std::pair<xAOD::CaloCluster::CaloSample, double>(mostEnergeticLayer, maxLayerClusterEnergy);
  }
} // Derivation Framework
