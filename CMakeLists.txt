################################################################################
# Package: DerivationFrameworkEoverP
################################################################################

# Declare the package name:
atlas_subdir( DerivationFrameworkEoverP )

# Declare the package's dependencies:
atlas_depends_on_subdirs( PUBLIC
                          GaudiKernel
                          External/GaudiInterfaces
                          Control/AthenBaseComps
                          External/AtlasROOT
                          PhysicsAnalysis/DerivationFramework/DerivationFrameworkInterfaces
                          Trigger/TrigEvent/TrigCaloEvent
                          Calorimeter/CaloIdentifier
                          Reconstruction/RecoTools/RecoToolInterfaces
                          Event/xAOD/xAODCaloEvent
                          PRIVATE
                          Event/xAOD/xAODEventInfo
                          Calorimeter/CaloEvent
                          Calorimeter/CaloUtils
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
                          Calorimeter/CaloDetDescr
)

# Find the needed external(s):
find_package( ROOT COMPONENTS Core RIO Hist Tree )

atlas_install_python_modules( python/*.py )
atlas_install_joboptions( share/*.py )

atlas_add_component(DerivationFrameworkEoverP DerivationFrameworkEoverP/*.h src/*.cxx src/components/*.cxx ${DerivationFrameworkEoverPDictSource}
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
                   LINK_LIBRARIES  ${ROOT_LIBRARIES} 
                   ${release_libs} GaudiKernel DerivationFrameworkInterfaces RecoToolInterfaces
)
