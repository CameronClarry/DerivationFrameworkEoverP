#! /usr/bin/env python
import commands
import subprocess as sp

# Joakim Olsson <joakim.olsson@cern.ch>

tag = '20190321_test3'
user = 'luadamek'

nFiles = 1
nFilesPerJob = 1

# If sub-jobs exceed the walltime limit, they will get killed. When you want to submit long running jobs (e.g., customized G4 simulation), submit them to sites where longer walltime limit is available by specifying the expected execution time (in second) to the --maxCpuCount option.
maxCpuCount = 252000 # 70 hrs ##172800 # 48 hrs

doBuild = True
doBuildAll = True

# inDSs  = ['data15_13TeV.00267358.physics_MinBias.recon.ESD.r7922']
          # 'data15_13TeV.00267599.physics_MinBias.recon.ESD.r7922',
          # 'data15_13TeV.00267359.physics_MinBias.recon.ESD.r7922',
          # 'data15_13TeV.00267367.physics_MinBias.recon.ESD.r7922',
          # 'data15_13TeV.00267385.physics_MinBias.recon.ESD.r7922',
          # 'data15_13TeV.00267360.physics_MinBias.recon.ESD.r7922']
# inDSs  = ['mc15_13TeV.428001.ParticleGun_single_piplus_logE0p2to2000.recon.ESD.e3501_s2832_r8014']

# outDSs  = ['data15_13TeV.00267358.physics_MinBias.DAOD_EOP.r7922']
           # 'data15_13TeV.00267599.physics_MinBias.DAOD_EOP.r7922',
           # 'data15_13TeV.00267359.physics_MinBias.DAOD_EOP.r7922',
           # 'data15_13TeV.00267367.physics_MinBias.DAOD_EOP.r7922',
           # 'data15_13TeV.00267385.physics_MinBias.DAOD_EOP.r7922',
           # 'data15_13TeV.00267360.physics_MinBias.DAOD_EOP.r7922']
# outDSs  = ['mc15_13TeV.428001.single_piplus_logE0p2to2000.DAOD_EOP.e3501_s2832_r8014']

inDSs = [\
"data17_13TeV.00341294.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
"mc16_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.recon.ESD.e3569_s3170_r10572"]

outDSs = [\
"data17_13TeV.00341294.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
"mc16_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.deriv.DAOD_EOP.e3569_s3170_r10572"]

setup = '--nFiles '+str(nFiles)+' --nFilesPerJob '+str(nFilesPerJob)+ ' --excludedSite ANALY_BNL_SHORT --maxCpuCount '+str(maxCpuCount)+' --useNewTRF --trf "Reco_tf.py --outputDAOD_EOPFile=%OUT.pool.root --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything" --individualOutDS'
print('setup: '+setup)

config = ''
print('config: '+config)

comFirst = 'pathena {} --outDS {} --inDS {} {}'
comLater = 'pathena {} --outDS {} --inDS {} --libDS LAST {}'

# Submit jobs to the grid with pathena
# https://twiki.cern.ch/twiki/bin/view/PanDA/PandaAthena
for i,inDS in enumerate(inDSs):
    outDS = 'user.'+user+'.'+outDSs[i]+'.'+tag
    print('Input dataset: '+inDS)
    print('Output dataset: '+outDS)
    if (i==0 and doBuild) or doBuildAll:
        command = comFirst.format(setup, outDS, inDS, config)
    else:
        command = comLater.format(setup, outDS, inDS, config)
    print(command)
    sp.call(command, shell=True)
