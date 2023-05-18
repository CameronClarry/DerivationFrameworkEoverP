/* 
 * @file     TrackCaloDecorator.h
 * @author   Millie McDonald <emcdonal@cern.ch> // Main author
 * @author   Joakim Olsson <joakim.olsson@cern.ch> // Modifications and cross-checks
 * @author   Lukas Adamek <Lukas.Adamek@cern.ch> //Modifications to include calibration hit information
 * @brief    A derivation for ATLAS Run II E/p analyses. Extrapolates all tracks to calorimeter and decorates them with cluster and cell energies.
 * Updated: 2 December 2015, Millie McDonald
 * Updated: 12 December 2016, Joakim Olsson
 */
#ifndef __TRACKCALODECORATOR_H
#define __TRACKCALODECORATOR_H

#include <string>
#include <vector>

#include "TH1F.h"
#include "TTree.h"

#include "AthenaBaseComps/AthAlgTool.h"
#include "MCTruthClassifier/IMCTruthClassifier.h"
#include "DerivationFrameworkInterfaces/IAugmentationTool.h"
#include "GaudiKernel/ToolHandle.h"
#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ITHistSvc.h"
#include "CaloIdentifier/CaloCell_ID.h"
#include "RecoToolInterfaces/IParticleCaloExtensionTool.h"
#include "xAODCaloEvent/CaloClusterContainer.h"
#include "xAODCaloEvent/CaloClusterChangeSignalState.h"
#include "CaloEvent/CaloClusterContainer.h"
#include "CaloEvent/CaloCluster.h"
#include "CaloEvent/CaloCellContainer.h"
#include "xAODTruth/TruthParticleContainer.h"
#include "CaloSimEvent/CaloCalibrationHitContainer.h"  

class TileTBID;

namespace Trk {
  class IExtrapolator;
  class Surface;
  class TrackParametersIdHelper;
}
namespace DerivationFramework {

  class TrackCaloDecorator : public AthAlgTool, public IAugmentationTool {
    public: 
      TrackCaloDecorator(const std::string& t, const std::string& n, const IInterface* p);

     std::vector<std::string> m_cutNames;
     const std::map<std::string, float> m_stringToCut = {
     {"025" , 0.025},
     {"050" , 0.050},
     {"075" , 0.075},
     {"100" , 0.100},
     {"125" , 0.125},
     {"150" , 0.150},
     {"175" , 0.175},
     {"125" , 0.125},
     {"150" , 0.150},
     {"175" , 0.175},
     {"200" , 0.200},
     {"225" , 0.225},
     {"250" , 0.250},
     {"275" , 0.275},
     {"300" , 0.300}
     };

     const unsigned int m_ncuts = m_stringToCut.size();
     const unsigned int m_nsamplings = CaloSampling::getNumberOfSamplings();

     std::vector<CaloSampling::CaloSample> m_caloSamplingNumbers;
     std::vector<unsigned int> m_caloSamplingIndices;
     std::map<CaloSampling::CaloSample, unsigned int> m_mapCaloSamplingToIndex;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_CellEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_LCWClusterEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterNonEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterEscapedActiveCalibHitEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterNonEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterInvisibleInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterEscapedInactiveCalibHitEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedActiveCalibHitEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundNonEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundInvisibleInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterPhotonBackgroundEscapedInactiveCalibHitEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleActiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedActiveCalibHitEnergy;

      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundNonEMInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundInvisibleInactiveCalibHitEnergy;
      std::vector< std::vector<SG::AuxElement::Decorator< float > > > m_cutToCaloSamplingIndexToDecorator_ClusterHadronicBackgroundEscapedInactiveCalibHitEnergy;

      std::vector<SG::AuxElement::Decorator< float > >  m_caloSamplingIndexToDecorator_extrapolTrackEta;
      std::vector<SG::AuxElement::Decorator< float > >  m_caloSamplingIndexToDecorator_extrapolTrackPhi;

      StatusCode initialize();
      StatusCode finalize();
      virtual StatusCode addBranches() const;

    private:

      std::vector<unsigned int> m_cutNumbers;
      std::map<unsigned int, float> m_cutNumberToCut;
      std::map<unsigned int, std::string> m_cutNumberToCutName;
      std::string m_sgName;
      std::string m_eventInfoContainerName;
      std::string m_trackContainerName;
      std::string m_caloClusterContainerName;


      std::string m_tileActiveHitCnt;
      std::string m_tileInactiveHitCnt;
      std::string m_tileDMHitCnt;
      std::string m_larInactHitCnt;
      std::string m_larActHitCnt;
      std::string m_larDMHitCnt;


      ToolHandle<Trk::IExtrapolator> m_extrapolator;
      ToolHandle<Trk::IParticleCaloExtensionTool> m_theTrackExtrapolatorTool;
      ToolHandle<IMCTruthClassifier> m_truthClassifier;
      Trk::TrackParametersIdHelper* m_trackParametersIdHelper;

      const TileTBID* m_tileTBID; 

      bool m_doCutflow;

      // Tree with run number, event number, lumi block, and nTrks
      TTree* m_tree;
      int m_runNumber;
      int m_eventNumber;
      int m_lumiBlock;
      int m_nTrks;
      int m_nTrks_pass;

      // cutflows
      TH1F* m_cutflow_evt;
      TH1F* m_cutflow_trk;
      TH1F* m_ntrks_per_event_all;
      TH1F* m_ntrks_per_event_pass_all;
      int m_cutflow_evt_all;
      int m_cutflow_evt_pass_all;
      int m_cutflow_trk_all;
      int m_cutflow_trk_pass_extrapolation;
      int m_cutflow_trk_pass_cluster_matching;
      int m_cutflow_trk_pass_cell_matching;
      int m_cutflow_trk_pass_loop_matched_clusters;
      int m_cutflow_trk_pass_loop_matched_cells;
      int m_cutflow_trk_pass_all;
      
      /** ReadHandleKey for the CaloClusterContainer, at LC scale, to be used as input */
      SG::ReadHandleKey<xAOD::CaloClusterContainer> m_caloCalClustersReadHandleKey{
          this,
              "calClustersName",
              "CaloCalTopoClusters",
              "ReadHandleKey for the CaloClusterContainer, at LC scale, to be used as "
                  "input"
      };


    public: 
      void getHitsSum(const CaloCalibrationHitContainer* hits,const  xAOD::CaloCluster* cl,  unsigned int particle_barcode, std::vector< std::vector<float> >& hitsMap) const;

      void getHitsSumAllBackground(const CaloCalibrationHitContainer* hits, const xAOD::CaloCluster* cl,  unsigned int particle_barcode, const xAOD::TruthParticleContainer* truthParticles, std::vector<int> sumForThesePDGIDs, std::vector<int> skipThesePDGIDs,  std::vector< std::vector<float> >& hitsMap) const;
  }; 
} // Derivation Framework
#endif 
