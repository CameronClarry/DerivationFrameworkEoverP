# Derivation Framework E/p #

A derivation for ATLAS Run II E/p analyses. Extrapolates all tracks to calorimeter and decorates them with cluster and cell energies.

Package created by: Millie McDonald

Modifications by: Joakim Olsson

Port to release 21: Lukas Adamek

## Setup

First we need to setup up our working directory

```
mkdir workdir && cd workdir
mkdir source && build && run
cd source
```

Setup AtlasDerivation and do a sparce checkout of athena.
```
asetup AtlasDerivation 21.0.19.8 here
lsetup git
git atlas init-workdir https://:@gitlab.cern.ch:8443/atlas/athena.git
#git atlas init-workdir https://:@gitlab.cern.ch:8443/luadamek/athena.git
cd athena
git atlas addpkg PrimaryDPDMaker
git fetch upstream
git checkout -b 21.0.19 release/21.0.19
cd ..
```

Get the Derivation Frameowrk, and modify the PrimaryDPDFlags.py file in the PrimaryDPDMaker

```
git clone https://github.com/luadamek/DerivationFrameworkEoverP
cp DerivationFrameworkEoverP/python/PrimaryDPDFlags_mod.py athena/PhysicsAnalysis/PrimaryDPDMaker/python/PrimaryDPDFlags.py
```

And finally, compile the project.
```
cd ../build
cmake ../source && make
source x86_64-slc6-gcc49-opt/setup.sh
```

## Running

###Running locally on lxplus
Download an example esd, and run over it
```
cd ../run
lsetup rucio
voms-proxy-init voms-atlas
rucio download data16_13TeV.00303499.physics_ZeroBias.recon.ESD.f716._lb0156._SFO-ALL._0001.1
Reco_tf.py --autoConfiguration='everything' --maxEvents 10 --inputESDFile data16_13TeV/data16_13TeV.00303499.physics_ZeroBias.recon.ESD.f716._lb0156._SFO-ALL._0001.1 --outputDAOD_EOPFile output_DAOD_EOP.root
```

### Example: submitting jobs to the grid

NB: This has not been tested in Release 21 yet. This is to be updated

```
cd run
lsetup panda
tag=test0
pathena --nFiles 1 --nFilesPerJob 1 --nEventsPerFile 1000 --maxCpuCount 252000 --useNewTRF --trf "Reco_tf.py --outputDAOD_EOPFile=%OUT.pool.root --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything --maxEvents 1000" --extOutFile cutflow.root --individualOutDS --outDS user.$USER.data15_13TeV.00267360.physics_MinBias.DAOD_EOP.r7922.$tag --inDS data15_13TeV.00267360.physics_MinBias.recon.ESD.r7922
```

To submit grid jobs for several datasets, see scripts in 'DerivationFrameworkEoverP/python'.
