# Derivation Framework E/p #

A derivation for ATLAS Run II E/p analyses. Extrapolates all tracks to calorimeter and decorates them with cluster and cell energies.

Package created by: Millie McDonald

Modifications by: Joakim Olsson

## Setup

```
mkdir DerivationFrameworkEoverPAthena; cd DerivationFrameworkEoverPAthena
setupATLAS
asetup 20.7.7.4,AtlasDerivation,here
cmt co PhysicsAnalysis/PrimaryDPDMaker
git clone https://github.com/jmrolsson/DerivationFrameworkEoverP
cp DerivationFrameworkEoverP/python/PrimaryDPDFlags_mod.py PhysicsAnalysis/PrimaryDPDMaker/python/PrimaryDPDFlags.py
cmt clean; cmt find_packages; cmt compile
```

## Running

### Example: running locally on lxplus

```
mkdir run; cd run
Reco_tf.py --autoConfiguration='everything' --maxEvents 1000 --inputESDFile testESD.root --outputDAOD_EOPFile output_DAOD_EOP.root
```

### Example: submitting jobs to the grid

```
cd run
lsetup panda
pathena --nFiles 1 --nFilesPerJob 1 --nEventsPerFile 1000 --maxCpuCount 252000 --useNewTRF --trf Reco_tf.py --outputAODFile=%OUT --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything --maxEvents 1000 --extOutFile cutflow.pool.root --individualOutDS --outDS user.jolsson.data15_13TeV.00267360.physics_MinBias.DAOD_EOP.r7922.test1 --inDS data15_13TeV.00267360.physics_MinBias.recon.ESD.r7922
```

To submit grid jobs for several datasets, see scripts in 'DerivationFrameworkEoverP/python'.
