#! /usr/bin/env python
import commands
import subprocess as sp

# Joakim Olsson <joakim.olsson@cern.ch>
# Lukas Adamek <Lukas.adamek@cern.ch>

tag = '030419_calibhitinfo'
user = 'luadamek'

nFilesPerJob = 1

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

#inDSs = ["mc15_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.recon.ESD.e3569_s2832_r7968"]
         #"mc15_13TeV.361021.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ1W.recon.ESD.e3569_s2832_r7968"

inDSs = [\
#"data17_13TeV.00341294.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"data17_13TeV.00341312.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"data17_13TeV.00341419.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"data17_13TeV.00341534.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"data17_13TeV.00341615.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"data17_13TeV.00341649.physics_MinBias.merge.DESDM_EOVERP.r11054_p3694",\
#"mc16_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.recon.ESD.e3569_s3170_r10572",\
#"mc16_13TeV.361021.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ1W.recon.ESD.e3569_s3170_r10572",\
#"mc16_13TeV.361022.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ2W.recon.ESD.e3668_s3170_r10572",\
"mc16_13TeV.428001.ParticleGun_single_piplus_logE0p2to2000.recon.ESD.e3501_s3007_r8917",\
"mc16_13TeV.428002.ParticleGun_single_piminus_logE0p2to2000.recon.ESD.e3501_s3007_r8917",\
]

outDSs = [\
#"data17_13TeV.00341294.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"data17_13TeV.00341312.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"data17_13TeV.00341419.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"data17_13TeV.00341534.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"data17_13TeV.00341615.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"data17_13TeV.00341649.physics_MinBias.deriv.DAOD_EOP.r11054_p3694",\
#"mc16_13TeV.361020.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ0W.deriv.DAOD_EOP.e3569_s3170_r10572",\
#"mc16_13TeV.361021.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ1W.deriv.DAOD_EOP.e3569_s3170_r10572",\
#"mc16_13TeV.361022.Pythia8EvtGen_A14NNPDF23LO_jetjet_JZ2W.deriv.DAOD_EOP.e3668_s3170_r10572",\
"mc16_13TeV.428001.ParticleGun_single_piplus_logE0p2to2000.deriv.DAOD_EOP.e3501_s3007_r8917",\
"mc16_13TeV.428002.ParticleGun_single_piminus_logE0p2to2000.deriv.DAOD_EOP.e3501_s3007_r8917",\
]

setup = '--nFilesPerJob '+str(nFilesPerJob)+' --excludedSite ANALY_BNL_SHORT,ANALY_SiGNET,ANALY_SiGNET_DIRECT,ANALY_ARNES,ANALY_ARNES_DIRECT,ANALY_RAL_ECHO --maxCpuCount '+str(maxCpuCount)+' --useNewTRF --trf "Reco_tf.py --outputDAOD_EOPFile=%OUT.pool.root --inputESDFile=%IN --ignoreErrors=True --autoConfiguration=everything" --individualOutDS'
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
