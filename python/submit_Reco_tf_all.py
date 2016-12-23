#! /usr/bin/env python
import commands
import subprocess as sp

# Joakim Olsson <joakim.olsson@cern.ch>

tag = '20161223_0'
user = 'jolsson'

nFilesPerJob = 3

# If sub-jobs exceed the walltime limit, they will get killed. When you want to submit long running jobs (e.g., customized G4 simulation), submit them to sites where longer walltime limit is available by specifying the expected execution time (in second) to the --maxCpuCount option.
maxCpuCount = 252000 # 70 hrs ##172800 # 48 hrs

doBuild = True
doBuildAll = True


inDSs  = ['data15_13TeV.00267358.physics_MinBias.recon.ESD.r7922',
          'data15_13TeV.00267599.physics_MinBias.recon.ESD.r7922',
          'data15_13TeV.00267359.physics_MinBias.recon.ESD.r7922',
          'data15_13TeV.00267367.physics_MinBias.recon.ESD.r7922',
          'data15_13TeV.00267385.physics_MinBias.recon.ESD.r7922',
          'data15_13TeV.00267360.physics_MinBias.recon.ESD.r7922']

outDSs  = ['data15_13TeV.00267358.physics_MinBias.DAOD_EOP.r7922',
           'data15_13TeV.00267599.physics_MinBias.DAOD_EOP.r7922',
           'data15_13TeV.00267359.physics_MinBias.DAOD_EOP.r7922',
           'data15_13TeV.00267367.physics_MinBias.DAOD_EOP.r7922',
           'data15_13TeV.00267385.physics_MinBias.DAOD_EOP.r7922',
           'data15_13TeV.00267360.physics_MinBias.DAOD_EOP.r7922']

setup = '--nFilesPerJob '+str(nFilesPerJob)+' --maxCpuCount '+str(maxCpuCount)+' --useNewTRF --trf "Reco_tf.py --outputAODFile=%OUT --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything" --extOutFile cutflow.pool.root --individualOutDS'
print 'setup: '+setup

config = ''
print 'config: '+config

comFirst = 'pathena {} --outDS {} --inDS {} {}'
comLater = 'pathena {} --outDS {} --inDS {} --libDS LAST {}'

# Submit jobs to the grid with pathena
# https://twiki.cern.ch/twiki/bin/view/PanDA/PandaAthena
for i,inDS in enumerate(inDSs):
    outDS = 'user.'+user+'.'+outDSs[i]+'.'+tag
    print 'Input dataset: '+inDS
    print 'Output dataset: '+outDS
    if (i==0 and doBuild) or doBuildAll:
        command = comFirst.format(setup, outDS, inDS, config)
    else:
        command = comLater.format(setup, outDS, inDS, config)
    sp.call('echo '+command, shell=True)
    sp.call(command, shell=True)
