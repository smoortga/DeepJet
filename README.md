In this reposiroty are the parts of DeepJET that are not CMS specific and live outside CMS software.

NTuple Production (CMSSW_8_1_0):
=============================================

First of all put this in your login script:
```
export X509_USER_PROXY=${HOME}/.gridproxy.pem
```

First create your desired CMSSW area:
```
cmsrel CMSSW_8_1_0
cd CMSSW_8_1_0/src
cmsenv
git cms-init
```

clone and initialize the DeepNTuples repo
```
git clone https://github.com/CMSDeepFlavour/DeepNTuples
cd DeepNTuples
git submodule init
git submodule update
cd ..
scram b -j 4
```

Now we are ready to prudce ntuples. We will use as an example the config “DeepNtuplizer.py” in “DeepNTuples/DeepNtuplizer/production” and run over the PhaseI sample which are summarized in “samples\_phase1.cfg” in the same directory. The output root files will NOT be stored in the output directory specified with the jobSub.py command (see below). Instead, a new directory will be created on the user eos (cernbox). A symlink will be provided in the output directory <myoutdir>. Note that the sample production can take a few hours.
```
cd DeepNTuples/DeepNtuplizer/production
jobSub.py --file samples_phase1.cfg DeepNtuplizer.py <myoutdir>
```

NOTE: for quick local testing, one can alter the DeepNtuplizer.py config to read in some relval or other testing file you are interested in and you can simply run “cmsRun DeepNtuplizer.py” in the “DeepNTuples/DeepNtuplizer/production” directory. Also specify the number of events you want to run over.

To check the job status of for example the “ntuple\_qcd\_170\_300\_phase1” (for naming see the samples\_pahse1.cfg):

```
cd <myoutdir>
check.py ntuple_qcd_170_300_phase1
```

if too many failures happened, one can resubmit these failed jobs with (using the same qcd\_170\_300 sample as an example):

```
check.py ntuple_qcd_170_300_phase1 --action resubmit
```

once all the samples have a sufficient amount of succeeded jobs, you can start merging all the output files together. In principle each directory that was created on eos should also contain two .txt files: train_val_samples.txt and test_samples.txt. They list respectively the samples that are used for training (and validating during the training) and those used for the separate testing afterwards. We will now perform two merges: one for training (a mixture of ttbar and qcd) and one for testing (pure ttbar, but one could make one for pure QCD as well). The merging should be rather quick with the “parallel” version of the merger. Count in the order of two hours for the training samples (less for the limited testing on ttbar).

For merging the training sample in batches of 400000 jets per file to a directory on eos called ‘<eos_merged_training_directory>’ (put it next to the directories made for each sample individually), do:

```
mergeSamples_parallel 400000 <eos_merged_training_directory> ntuple_*/train_val_samples.txt
```

For merging the testing (ttbar) sample in batches of 400000 jets per file to a directory called ‘<eos_merged_testing_ttbar_directory>’, do:

```
mergeSamples_parallel 400000 <eos_merged_testing_ttbar_directory> ntuple_ttbar_phase1/test_samples.txt
```

now everything should be safely stored to eos. You can check the root files manually to see if all went well.




Performing a training:
=============================================

Install miniconda (with python 2.7) in your afs work directory. For me it is in /afs/cern.ch/work/s/smoortga/miniconda3. Follow instructions from: https://conda.io/docs/install/quick.html#linux-miniconda-install 

Now we get the DeepJet repository (the one from Markus is the most up-to-date) and prepare the environment (this needs to be done only once):

```
git clone https://github.com/mstoye/DeepJet.git
cd DeepJet/environment
./setupEnv.sh deepjetLinux3.conda
./setupEnv.sh deepjetLinux3.conda gpu
source lxplus_env.sh
cd ../modules
make
```


This finished the environment setup. Now everytime you log on to lxplus7 you can do:

```
cd DeepJet/environment
source lxplus_env.sh
```

or if you are logged in to the GPUs you instead have to do:

```
source gpu_env.sh
```

Now convert training and testing files form .root to numpy format. It is the quickest to save these numpy files to the /data/ repo on the GPU clusters. You also need to specify a TrainData model that specifies which variables you want to use etc. Predefined examples can be found in DeepJet/modules/TrainData\_*. Let us use as an example “TrainData\_deepCSV\_PF” defined in DeepJet/modules/TrainData\_deepCSV\_PF.py.

```
ssh tlab-gpu-nv-05
cd DeepJet/environment
source gpu_env.sh
cd ../convertFromRoot
python convertFromRoot.py –i <eos_merged_training_directory>/samples.txt –o /data/<username>/<name_of_output_directory_to_store_numpy_files> -c TrainData_deepCSV_PF
```

Now also do it for the testing data. Notice that here you don’t need to specify the TrainData model (so you don't need to use -c), but instead you specify the “dataCollection.dc” file that was produced in your output directory of the previous step. Use for this purpose the --testdatafor argument:

```
python convertFromRoot.py –i <eos_merged_testing_ttbar_directory>/samples.txt –o /data/<username>/<name_of_output_directory_to_store_testing_numpy_files> --testdatafor /data/<username>/<name_of_output_directory_to_store_training_numpy_files>/dataCollection.dc
```

Now the data are ready for the training. We will use for this example the DeepJet/Train/DeepJetTrain\_PF.py script which uses the network with the name “Dense\_model\_broad” located in DeepJet/modules/DeepJet\_models.py. I use nohup to run in the background. The training took around 10 hours for 100 epochs. The output of the training is stored in the defined output directory <outdir\_train>.

```
cd ../Train
nohup python –u DeepJetTrain_PF.py /data/<username>/<name_of_output_directory_to_store_training_numpy_files>/dataCollection.dc <outdir_train>
```

Finally after the training is done you can find the losses in <outdir\_train>/losses.log and the trained model with the lowest validation loss in <outdir\_train>/KERAS\_check\_best\_model.h5 
You can now run the testing on the pure ttbar with:

```
python predict.py <name_of_output_directory_to_store_details_of_the_training>/KERAS_check_best_model.h5 /data/<username>/<name_of_output_directory_to_store_testing_numpy_files>/dataCollection.dc <outdir_test>
```

This produces in <outdir\_test> a file named tree_association.txt that can be used to produce ROC curves with the scripts that can be found in DeepJet/Train/Plotting/








 





 












