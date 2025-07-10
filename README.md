# Derivation Framework E/p #

A derivation for ATLAS Run II E/p analyses. Extrapolates all tracks to calorimeter and decorates them with cluster and cell energies.

Package created by: Millie McDonald

Modifications by: Joakim Olsson, Lukas Adamek, Matt Leblanc

Port to release 21: Lukas Adamek

## Setup in Release 22

First we need to setup up our working directory and Athena

```
setupATLAS -c centos7
mkdir workdir && cd workdir
mkdir source build run
cd source
asetup Athena,22.0.104,here
```

Clone the Derivation Framework, and give the project a CMakeLists

```
git clone https://github.com/CameronClarry/DerivationFrameworkEoverP.git
cp DerivationFrameworkEoverP/UpperCMakeLists.txt CMakeLists.txt
```

And finally, compile the project.
```
cd ../build
cmake ../source && make
source x86_64-centos7-gcc11-opt/setup.sh
```

## Running in Release 22

On subsequent sessions, set up the proper running environment using:
```
cd workdir
setupATLAS -c centos7
cd source
asetup
cd ../build
# If you will be running locally
source x86_64-centos7-gcc11-opt/setup.sh
cd ../run
# If you are submitting to the grid
lsetup panda
cd ../
```

### Example: Running locally on lxplus
Run over example ESD
```
# Running on data:
rucio download ESD.29797580._000004.pool.root.1
python ../source/DerivationFrameworkEoverP/python/derivation.py --input_files /path/to/data/esd.root --isData --maxEvents 100 --nthreads 1
# Running on MC:
python ../source/DerivationFrameworkEoverP/python/derivation.py --input_files /path/to/mc/esd.root --maxEvents 100 --nthreads 1
```
Use ``python derivation.py --help`` to see a full list of options.

### Example: Submit to grid
```
# Make the appropriate changes for the dataset you are running on
prun --bexec "ls;mkdir build;cd build;cmake ../source;make;cd ../" --exec "ls;source build/x86_64-centos7-gcc11-opt/setup.sh;python source/DerivationFrameworkEoverP/python/derivation.py --isData --maxEvents=\"-1\" --nthreads 1 --input_files %IN" --inDS data17_13TeV.00341312.physics_ZeroBias.merge.ESD.r16331_p6686 --output myDAOD_EOP.pool.root --outDS eop.derivation.$(uuidgen) --athenaTag=Athena,22.0.104 --excludeFile ./build/ --cpuTimePerEvent 220
```

## Setup in Release 21

First we need to setup up our working directory

```
mkdir workdir && cd workdir
mkdir source && mkdir build && mkdir run
cd source
```

Setup AtlasDerivation and do a sparce checkout of athena.
```
asetup Athena,21.0.74,here
lsetup git
git atlas init-workdir https://:@gitlab.cern.ch:8443/atlas/athena.git
cd athena
git atlas addpkg PrimaryDPDMaker
git atlas addpkg TrkVertexAnalysisUtils
git atlas addpkg xAODBPhys
git fetch upstream
git checkout -b 21.0.74 release/21.0.74
cd ..
echo "- athena/Projects/WorkDir" >> package_filters.txt
```

Clone the Derivation Framework, and modify the PrimaryDPDFlags.py file in the PrimaryDPDMaker

```
git clone https://github.com/luadamek/DerivationFrameworkEoverP.git
cp DerivationFrameworkEoverP/ModifiedAthena/PrimaryDPDFlags_mod.py athena/PhysicsAnalysis/PrimaryDPDMaker/python/PrimaryDPDFlags.py
cp DerivationFrameworkEoverP/ModifiedAthena/V0Tools_MOD.cxx athena/Tracking/TrkVertexFitter/TrkVertexAnalysisUtils/src/V0Tools.cxx
cp DerivationFrameworkEoverP/ModifiedAthena/V0Tools_MOD.h athena/Tracking/TrkVertexFitter/TrkVertexAnalysisUtils/TrkVertexAnalysisUtils/V0Tools.h
cp DerivationFrameworkEoverP/ModifiedAthena/BPhysHelper_MOD.cxx athena/Event/xAOD/xAODBPhys/Root/BPhysHelper.cxx
cp DerivationFrameworkEoverP/ModifiedAthena/BPhysHelper_MOD.h athena/Event/xAOD/xAODBPhys/xAODBPhys/BPhysHelper.h
cp DerivationFrameworkEoverP/ModifiedAthena/BPhysHypoHelper_MOD.h  athena/Event/xAOD/xAODBPhys/xAODBPhys/BPhysHypoHelper.h
cp DerivationFrameworkEoverP/ModifiedAthena/BPhysHypoHelper_MOD.cxx athena/Event/xAOD/xAODBPhys/Root/BPhysHypoHelper.cxx
```

And finally, compile the project.
```
cd ../build
cmake ../source && make
source x86_64-slc6-gcc62-opt/setup.sh
```



## Running in Release 21

### Example: Running locally on lxplus
Download an example esd, and run over it
```
cd ../run
lsetup rucio
voms-proxy-init -voms atlas
rucio download data17_13TeV:DESDM_EOVERP.16059171._000170.pool.root.1
Reco_tf.py --autoConfiguration='everything' --maxEvents 100 --inputESDFile data17_13TeV/DESDM_EOVERP.16059171._000170.pool.root.1 --outputDAOD_EOPFile output_DAOD_EOP.root
```

## Setup in Release 20.7

```
mkdir DerivationFrameworkEoverPAthena; cd DerivationFrameworkEoverPAthena
setupATLAS
asetup 20.7.7.4,AtlasDerivation,here
cmt co PhysicsAnalysis/PrimaryDPDMaker
git clone git@github.com:luadamek/DerivationFrameworkEoverP.git
cp DerivationFrameworkEoverP/python/PrimaryDPDFlags_mod_r207.py PhysicsAnalysis/PrimaryDPDMaker/python/PrimaryDPDFlags.py
cmt clean; cmt find_packages; cmt compile
```

## Running in release 20.7
Download an example esd, and run over it
```
mkdir run && cd run
voms-proxy-init -voms atlas
rucio download data17_13TeV:DESDM_EOVERP.16059171._000170.pool.root.1
Reco_tf.py --autoConfiguration='everything' --maxEvents 100 --inputESDFile data17_13TeV/DESDM_EOVERP.16059171._000170.pool.root.1 --outputDAOD_EOPFile output_DAOD_EOP.root
```


### Example: submitting jobs to the grid

```
cd run
lsetup panda
python ../source/DerivationFrameworkEoverP/python/submit_Reco_tf_test.py
```

To submit grid jobs for several datasets, see scripts in 'DerivationFrameworkEoverP/python'.
