################################################################################
# Package: DerivationFrameworkEoverP
################################################################################

# Declare the package name:
atlas_subdir( DerivationFrameworkEoverP )

# Declare the package's dependencies:
atlas_depends_on_subdirs( PUBLIC
			  GaudiKernel
			  Event/xAOD/xAODMuon
			  Control/DataModel
			  Control/AthenaBaseComps
			  PhysicsAnalysis/DerivationFramework/DerivationFrameworkInterfaces
			  PhysicsAnalysis/JpsiUpsilonTools
			  Event/xAOD/xAODTracking
			  Event/xAOD/xAODBPhys
			  Control/AthenaKernel
			  Reconstruction/RecoTools/RecoToolInterfaces
			  Event/EventPrimitives
			  PhysicsAnalysis/BPhys/BPhysTools
              External/GaudiInterfaces
              External/AtlasROOT
              PhysicsAnalysis/DerivationFramework/DerivationFrameworkInterfaces
              Trigger/TrigEvent/TrigCaloEvent
              Calorimeter/CaloIdentifier
              Reconstruction/RecoTools/RecoToolInterfaces
              Event/xAOD/xAODCaloEvent
              Event/xAOD/xAODMuon
			  PRIVATE
			  Event/xAOD/xAODBase
			  Event/xAOD/xAODMetaData
			  Tracking/TrkVertexFitter/TrkVertexAnalysisUtils
			  Database/AthenaPOOL/AthenaPoolUtilities
			  Event/xAOD/xAODPrimitives
			  Calorimeter/CaloEvent
			  Reconstruction/RecoTools/TrackToCalo
			  Reconstruction/RecoTools/TrackToVertex
			  Tracking/TrkEvent/TrkCaloExtension
			  InnerDetector/InDetConditions/InDetBeamSpotService
			  InnerDetector/InDetRecTools/InDetTrackSelectionTool
              Event/xAOD/xAODEventInfo
              Calorimeter/CaloEvent
              Calorimeter/CaloSimEvent
              Calorimeter/CaloUtils
			  PhysicsAnalysis/JpsiUpsilonTools
			  TrkVKalVrtFitter/TrkVKalVrtFitter
			  TileCalorimeter/TileEvent
              Tracking/TrkExtrapolation/TrkExInterfaces
              Tracking/TrkEvent/TrkParamters
			  Tracking/TrkEvent/TrkTrack
       	      Event/xAOD/xAODTracking 
              Calorimeter/CaloTrackingGeometry
              Tracking/TrkDetDescr/TrkSurfaces
              DetectorDescription/GeoPrimitives
              Tracking/TrkEvent/TrkCaloExtension
              Tracking/TrkEvent/TrkParamtersIdentificationHelpers
			  Tracking/TrkVertexFitter/TrkVertexAnalysisUtils
			  InnerDetector/InDetConditions/InDetBeamSpotService
			  PhysicsAnalysis/BPhys/BPhysTools
			  Trigger/TrigAnalysis/TrigDecisionTool
              Calorimeter/CaloDetDescr
			  InnerDetector/InDetRecAlgs/InDetV0Finder
              PhysicsAnalysis/MCTruthClassifier
)

# Find the needed external(s):
find_package( ROOT COMPONENTS Core RIO Hist Tree )
find_package(HepPDT REQUIRED)

atlas_install_python_modules( python/*.py )
atlas_install_joboptions( share/*.py )

atlas_add_component(DerivationFrameworkEoverP DerivationFrameworkEoverP/*.h src/*.cxx src/components/*.cxx
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS} ${HEPPDT_INCLUDE_DIRS}
      		       LINK_LIBRARIES  ${ROOT_LIBRARIES}  ${HEPPDT_LIBRARIES}
                   ${release_libs} GaudiKernel xAODEventInfo TrkExInterfaces
                   RecoToolInterfaces xAODMuon JpsiUpsilonTools DataModel EventPrimitives xAODBPhys
                   PRIVATE_LINK_LIBRARIES TrkVertexAnalysisUtils TrackToVertex InDetV0FinderLib InDetBeamSpotServiceLib 
                   TrkVertexAnalysisUtilsLib TrkVKalVrtFitterLib CaloSimEvent MCTruthClassifier
)
