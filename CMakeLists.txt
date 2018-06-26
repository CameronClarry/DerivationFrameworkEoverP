################################################################################
# Package: DerivationFrameworkEoverP
################################################################################

# Declare the package name:
atlas_subdir( DerivationFrameworkEoverP )

set( release_libs xAODTriggerCnvLib )

if( AnalysisBase_VERSION AND AnalysisBase_VERSION VERSION_GREATER 21.0 )
  # 21.X
  set( release_libs xAODTriggerCnvLib )
elseif( AnalysisTop_VERSION AND AnalysisTop_VERSION VERSION_GREATER 21.0 )
  # AnalysisTop 21.X
  set( release_libs xAODTriggerCnvLib )
else()
  # 2.6.X
  set( release_libs xAODTriggerCnv )
endif()

# Declare the package's dependencies:
atlas_depends_on_subdirs( PUBLIC
                          PhysicsAnalysis/D3PDTools/EventLoop
                          Control/AthContainers
                          Control/AthToolSupport/AsgTools
                          Control/xAODRootAccess
                          DataQuality/GoodRunsLists
                          Event/xAOD/xAODBase
                          Event/xAOD/xAODCaloEvent
                          Event/xAOD/xAODCutFlow
                          Event/xAOD/xAODEgamma
                          Event/xAOD/xAODEventInfo
                          Event/xAOD/xAODJet
                          Event/xAOD/xAODMetaDataCnv
                          Event/xAOD/xAODMissingET
                          Event/xAOD/xAODMuon
                          Event/xAOD/xAODTau
                          Event/xAOD/xAODTracking
                          Event/xAOD/xAODTrigger
                          Event/xAOD/xAODTriggerCnv
                          Event/xAOD/xAODTruth
                          TileCalorimeter/TileEvent
                          Calorimeter/CaloTrackingGeometry ##       InnerDetector/InDetRecTools
                          #PhysicsAnalysis/DerivationFramework
                          PhysicsAnalysis/PrimaryDPDMaker ##this is needed to work
                          Reconstruction/RecoTools/RecoToolInterfaces
                          PhysicsAnalysis/D3PDTools/EventLoopGrid
                          InnerDetector/InDetRecTools/InDetTrackSelectionTool
                          PhysicsAnalysis/AnalysisCommon/AssociationUtils
                          PhysicsAnalysis/AnalysisCommon/IsolationSelection
                          PhysicsAnalysis/AnalysisCommon/PATInterfaces
                          PhysicsAnalysis/AnalysisCommon/PMGTools
                          PhysicsAnalysis/AnalysisCommon/PileupReweighting
                          PhysicsAnalysis/D3PDTools/RootCoreUtils
                          PhysicsAnalysis/D3PDTools/SampleHandler
                          PhysicsAnalysis/ElectronPhotonID/ElectronEfficiencyCorrection
                          PhysicsAnalysis/ElectronPhotonID/ElectronPhotonFourMomentumCorrection
                          PhysicsAnalysis/ElectronPhotonID/ElectronPhotonSelectorTools
                          PhysicsAnalysis/ElectronPhotonID/IsolationCorrections
                          PhysicsAnalysis/ElectronPhotonID/ElectronPhotonShowerShapeFudgeTool
                          PhysicsAnalysis/ElectronPhotonID/PhotonEfficiencyCorrection
                          PhysicsAnalysis/JetMissingEtID/JetSelectorTools
                          PhysicsAnalysis/Interfaces/MuonAnalysisInterfaces
                          PhysicsAnalysis/MuonID/MuonIDAnalysis/MuonMomentumCorrections
                          PhysicsAnalysis/MuonID/MuonIDAnalysis/MuonEfficiencyCorrections
                          PhysicsAnalysis/TauID/TauAnalysisTools
                          Reconstruction/Jet/JetAnalysisTools/JetTileCorrection
                          Reconstruction/Jet/JetCalibTools
                          Reconstruction/Jet/JetJvtEfficiency
                          Reconstruction/Jet/JetMomentTools
                          Reconstruction/Jet/JetResolution
                          Reconstruction/Jet/JetSubStructureUtils
                          Reconstruction/Jet/JetUncertainties
                          Reconstruction/MET/METInterface
                          Reconstruction/MET/METUtilities
                          Tools/PathResolver
                          Trigger/TrigAnalysis/TrigDecisionTool
                          Trigger/TrigAnalysis/TriggerMatchingTool
                          Trigger/TrigConfiguration/TrigConfxAOD
                          ${release_depends}
)

# Find the needed external(s):
find_package( ROOT COMPONENTS Core RIO Hist Tree )

# build a dictionary for the library
#atlas_add_root_dictionary ( DerivationFrameworkEoverPLib DerivationFrameworkEoverPDictSource
#                            ROOT_HEADERS DerivationFrameworkEoverP/*.h Root/LinkDef.h
#                            EXTERNAL_PACKAGES ROOT
#)

# build a shared library
#atlas_add_library( DerivationFrameworkEoverPLib DerivationFrameworkEoverP/*.h Root/*.h Root/*.cxx ${DerivationFrameworkEoverPDictSource}
#                   PUBLIC_HEADERS DerivationFrameworkEoverP PrimaryDPDMaker
#                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
#                   LINK_LIBRARIES  ${ROOT_LIBRARIES} 
#                   xAODBase xAODRootAccess
#                   xAODEventInfo GoodRunsListsLib PileupReweightingLib PATInterfaces
#                   PathResolver xAODTau xAODJet xAODMuon 
#                   xAODEgamma TileEvent CaloTrackingGeometry
#                   xAODTracking xAODTruth MuonMomentumCorrectionsLib
#                   #DerivationFrameworkLib
#                   MuonEfficiencyCorrectionsLib
#                   MuonSelectorToolsLib JetCalibToolsLib
#                   JetSelectorToolsLib AthContainers
#                   ElectronPhotonFourMomentumCorrectionLib
#                   ElectronEfficiencyCorrectionLib ElectronPhotonSelectorToolsLib
#                   IsolationSelectionLib IsolationCorrectionsLib
#                   ElectronPhotonShowerShapeFudgeToolLib RecoToolInterfaces
#                   METUtilitiesLib METInterface
#                   AsgTools xAODMissingET JetResolutionLib
#                   TrigDecisionToolLib xAODCutFlow
#                   TriggerMatchingToolLib xAODMetaDataCnv xAODMetaData
#                   ${release_libs}
#)

# Install files from the package:
atlas_install_python_modules( python/*.py )
#atlas_install_joboptions( share/*.py )
atlas_install_joboptions( share/*.py )
#atlas_install_data( data/* )

atlas_add_component(DerivationFrameworkEoverP DerivationFrameworkEoverP/*.h Root/*.h Root/*.cxx Root/components/*.cxx ${DerivationFrameworkEoverPDictSource}
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
                   LINK_LIBRARIES  ${ROOT_LIBRARIES} 
                   xAODBase xAODRootAccess
                   xAODEventInfo GoodRunsListsLib PileupReweightingLib PATInterfaces
                   PathResolver xAODTau xAODJet xAODMuon 
                   xAODEgamma TileEvent CaloTrackingGeometry
                   xAODTracking xAODTruth MuonMomentumCorrectionsLib
                   #DerivationFrameworkLib
                   MuonEfficiencyCorrectionsLib
                   MuonSelectorToolsLib JetCalibToolsLib
                   JetSelectorToolsLib AthContainers
                   ElectronPhotonFourMomentumCorrectionLib
                   ElectronEfficiencyCorrectionLib ElectronPhotonSelectorToolsLib
                   IsolationSelectionLib IsolationCorrectionsLib
                   ElectronPhotonShowerShapeFudgeToolLib RecoToolInterfaces
                   METUtilitiesLib METInterface
                   AsgTools xAODMissingET JetResolutionLib
                   TrigDecisionToolLib xAODCutFlow
                   TriggerMatchingToolLib xAODMetaDataCnv xAODMetaData
                   ${release_libs}
)

### Install python packages
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
#include(SetupPythonPackage)

# Install argcomplete:
#setup_python_package( argcomplete
#   "https://pypi.python.org/packages/3b/db/c524f0d72842b44b179cc50d4257f1e72f447fef0919556e8b28a9b0f80f/argcomplete-1.9.2.tar.gz"
#   "13243e0b88102f13f7537f5aa70a48c0"
#   PATCH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch-setup.cfg setup.cfg
#)

#add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/EoverPDeriv_env_copied
#                   COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup.sh >> ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/setup.sh
#                   COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/EoverPDeriv_env_copied
#)
#add_custom_target(Setup_EoverPDeriv_Environment ALL DEPENDS ${CMAKE_BINARY_DIR}/EoverPDeriv_env_copied)
#execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/EoverPDeriv_env_copied)
