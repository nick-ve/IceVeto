////////////////////////////////////////////////////////
// Macro to attach and open an IceCube event file
// for interactive investigation.
//
// To run this macro, just do ($ is prompt)
//
// $root open.cc
//
// of from within a ROOT session
//
// root> .x open.cc
//
//--- NvE 18-jun-2004 Utrecht University
////////////////////////////////////////////////////////
{
 gSystem->Load("ncfspack");
 gSystem->Load("icepack");

 // Access to the input data
 TChain* data=new TChain("T");
 data->Add("*.icepack");

 // Define a pointer for an event
 IceEvent* evt=0;

 // Branch in the tree for the event input
 data->SetBranchAddress("IceEvent",&evt);

 cout << endl;
 cout << " *READ* nentries : " << data->GetEntries() << endl;
 cout << endl;

 // The main processing job to hold optional user tasks
 NcJob* job=new NcJob("Analyse","Optional user tasks for IcePack analysis of IceCube event data");

 // Optionally add the various processors as subtasks to the main job

 cout << " Use data->GetEntry(i) to load the i-th entry." << endl;
 cout << "  The event object is called evt " << endl;
 cout << " Task(s) can be executed via the command job->ProcessObject(evt) " << endl;
 cout << "  Use Browser window to (de)activate sub-tasks " << endl;
 cout << " Hits can be displayed via e.g. evt->DisplayHits(\"IceGOM\",\"ADC\",-1,1,7,kWhite) " << endl;
 cout << " All tracks can be displayed via reco->Display(evt) " << endl;
 cout << "  Specific tracks can be removed like e.g. evt->RemoveTracks(\"IceDwalk\") " << endl;
 cout << "  Track display can be cleaned by reco->Refresh(-1) " << endl;
 cout << "  A specific track (pointer tx) can be displayed by  reco->Display(tx) " << endl;
 cout << "  An array (arr) of selected tracks can be displayed by reco->Display(arr) " << endl;
 cout << endl;
 
 NcHelix* reco=new NcHelix();
 reco->Refresh(-1);
 reco->SetMarker(21);
 reco->UseEndPoint(0);
 reco->SetTofmax(5e-6);
}
