#! /usr/bin/env python
import commands
import subprocess as sp

# Joakim Olsson <joakim.olsson@cern.ch>

tag = '20160209_alpha1'
user = 'jolsson'

nFilesPerJob = 3

# If sub-jobs exceed the walltime limit, they will get killed. When you want to submit long running jobs (e.g., customized G4 simulation), submit them to sites where longer walltime limit is available by specifying the expected execution time (in second) to the --maxCpuCount option.
maxCpuCount = 252000 # 70 hrs ##172800 # 48 hrs

doBuild = True
doBuildAll = True

# inDSs  = ['data15_13TeV.00267358.physics_MinBias.recon.ESD.r7922',
#           'data15_13TeV.00267599.physics_MinBias.recon.ESD.r7922',
#           'data15_13TeV.00267359.physics_MinBias.recon.ESD.r7922',
#           'data15_13TeV.00267367.physics_MinBias.recon.ESD.r7922',
#           'data15_13TeV.00267385.physics_MinBias.recon.ESD.r7922',
#           'data15_13TeV.00267360.physics_MinBias.recon.ESD.r7922']
#
# outDSs  = ['data15_13TeV.00267358.physics_MinBias.DAOD_EOP.r7922',
#            'data15_13TeV.00267599.physics_MinBias.DAOD_EOP.r7922',
#            'data15_13TeV.00267359.physics_MinBias.DAOD_EOP.r7922',
#            'data15_13TeV.00267367.physics_MinBias.DAOD_EOP.r7922',
#            'data15_13TeV.00267385.physics_MinBias.DAOD_EOP.r7922',
#            'data15_13TeV.00267360.physics_MinBias.DAOD_EOP.r7922']

# inDSs  = ['data15_13TeV.00267599.physics_MinBias.recon.ESD.r7922']
# outDSs = ['data15_13TeV.00267599.physics_MinBias.DAOD_EOP.r7922']

# inDSs = ['mc15_13TeV.361203.Pythia8_A2_MSTW2008LO_ND_minbias.recon.ESD.e3639_s2601_s2132_r7728',
#          'mc15_13TeV.361204.Pythia8_A2_MSTW2008LO_SD_minbias.recon.ESD.e3639_s2601_s2132_r7728',
#          'mc15_13TeV.361205.Pythia8_A2_MSTW2008LO_DD_minbias.recon.ESD.e3639_s2601_s2132_r7728']

inDSs = ["mc15_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.recon.ESD.e3569_s2832_r7968"]
         #"mc15_13TeV.361021.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ1W.recon.ESD.e3569_s2832_r7968"

# outDSs = ['mc15_13TeV.361203.Pythia8_A2_MSTW2008LO_ND_minbias.DAOD_EOP.e3639_s2601_s2132_r7728',
#           'mc15_13TeV.361204.Pythia8_A2_MSTW2008LO_SD_minbias.DAOD_EOP.e3639_s2601_s2132_r7728',
#           'mc15_13TeV.361205.Pythia8_A2_MSTW2008LO_DD_minbias.DAOD_EOP.e3639_s2601_s2132_r7728']

outDSs = ["mc15_13TeV.361020.JZ0W.DAOD_EOP.e3569_s2832_r7968"]
         # "mc15_13TeV.361020.JZ1W.DAOD_EOP.e3569_s2832_r7968"

setup = '--nFilesPerJob '+str(nFilesPerJob)+' --maxCpuCount '+str(maxCpuCount)+' --useNewTRF --trf "Reco_tf.py --outputDAOD_EOPFile=%OUT.pool.root --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything" --extOutFile cutflow.root --individualOutDS'
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
    print(command)
    sp.call(command, shell=True)
