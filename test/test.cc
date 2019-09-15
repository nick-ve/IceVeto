////////////////////////////////////////////////////////
// Macro to test the new IceVeto functionality
//--- NvE 09-jun-2016 IIHE-VUB Brussels
/////////////////////////////////////////////
{
 gSystem->Load("ncfspack");
 gSystem->Load("icepack");

 gROOT->LoadMacro("IceVeto.cxx+");

 // Access to the input data
 TChain* data=new TChain("T");
 data->Add("*.icepack");

 // Define a pointer for an event
 IceEvent* evt=0;

 // Branch in the tree for the event input
 data->SetBranchAddress("IceEvent",&evt);

 // The main data processing job
 NcJob* job=new NcJob("NcJob","Processing of the IceCube event ROOT data");

 IceVeto* veto=new IceVeto();
 veto->ActivateVetoSystem("IceTop86");
// veto->ActivateVetoSystem("HESE86");
// veto->ActivateVetoSystem("Start86");
 veto->Data(1);

 NcEventSelector* sel=new NcEventSelector();
 sel->SetLogic("and");
 sel->SetSelector("event");
 sel->SetRange("event","veto",0,0);

 // Add the various processors as subtasks to the main job
 job->Add(veto);
 job->Add(sel);

 Int_t nen=(int)data->GetEntries();
 cout << endl;
 cout << " *READ* nentries : " << nen << endl;
 cout << endl;

 // Set number of entries for testing purposes
 nen=100;
 cout << endl;
 cout << " *PROCESS* nentries : " << nen << endl;
 cout << endl;

 Int_t nprocessed=0;
 Int_t nreject=0;
 Int_t naccept=0;
 Int_t nunknown=0;
 Int_t isel=0;
 Int_t nbytes=0;
 TObjArray* hits=new TObjArray();
 TObjArray* ordered=new TObjArray();
 NcDevice scanner;
 Double_t sum;
 Double_t thres;
 Double_t tmean,wtmean,tmedian,wtmedian,tstart;
 Int_t i1,i2;
 for (Int_t ien=0; ien<nen; ien++)
 {

  nbytes=data->GetEntry(ien);

  if (!evt || nbytes<=0) continue;

//  evt->GetHits("IceIDOM",hits);
  evt->GetHits("IceIDOM",hits,"SLC",-2);
  scanner.SortHits("LE",1,hits,8,1,ordered); // Sort hits with increasing hit time

  sum=scanner.SumSignals("ADC",8,ordered);

  // Test mean hit and start time determination
  tmedian=evt->GetCVAL(ordered,"LE","none",8,1);
  wtmedian=evt->GetCVAL(ordered,"LE","ADC",8,1);
  tmean=evt->GetCVAL(ordered,"LE","none",8,2);
  wtmean=evt->GetCVAL(ordered,"LE","ADC",8,2);

 thres=0.05*sum;
 if (thres<3) thres=3;
 tstart=scanner.SlideWindow(ordered,thres,3000,"LE",8,"ADC",8,&i1,&i2);  

cout << " %% tmedian:" << tmedian << " wtmedian:" << wtmedian
     << " tmean:" << tmean << " wtmean:" << wtmean << " tstart:" << tstart << " Qtot:" << sum << " thres:" << thres << endl;
cout << " Number of ordered hits:" << ordered->GetEntries() << " Found window i1:" << i1 << " i2:" << i2 << endl;
/****
for (Int_t i=i1; i<=i2; i++)
{
 if (i<0) break;
 NcSignal* sx=(NcSignal*)ordered->At(i);
 if (sx) sx->Data();
}
****/
  // Don't investigate events with Qtot<6000 pe
//@@@  if (sum<6000) continue;

  nprocessed++;

  job->ProcessObject(evt);

  isel=0;
  NcDevice* devx=evt->GetDevice("NcEventSelector");
  if (devx) isel=devx->GetSignal("Select");

  if (!isel) nunknown++;
  if (isel>0) naccept++;
  if (isel<0) nreject++;

evt->GetIdDevice(1,"NcVeto")->Data();
 }

 cout << endl;
 cout << " *** Event selection summary ***" << endl;
 cout << " processed:" << nprocessed << " accepted:" << naccept << " unknown:" << nunknown << " rejected:" << nreject << endl; 
}
