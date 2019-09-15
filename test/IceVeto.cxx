/*******************************************************************************
 * Copyright(c) 2003, IceCube Experiment at the South Pole. All rights reserved.
 *
 * Author: The IceCube NCFS-based Offline Project.
 * Contributors are mentioned in the code where appropriate.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation strictly for non-commercial purposes is hereby granted
 * without fee, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in the supporting documentation.
 * The authors make no claims about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *******************************************************************************/

// $Id$

///////////////////////////////////////////////////////////////////////////
// Class IceVeto
// TTask derived processor to perform (self)vetoing of events.
//
// In case an event has been rejected by an NcEventSelector (based) processor,
// this task (and its sub-tasks) is not executed.
//
// This task takes the current event in memory and uses the attached
// OM database to access the various calibration functions.
// A specific OM database may be attached by means of the SetOMdbase()
// or SetCalibFile() memberfunctions.
// Further details about the OM database can be found in the docs
// of IceCal2Root and IceDB2Root.
//
// In the calibration procedure, all event data in memory is scanned and
// replaced by calibrated data if a calibration function is present.
// When data is succesfully calibrated, the corresponding de-calibration
// function is stored in the event data at the appropriate place to allow
// access to uncalibrated data as well (see NcSignal::GetSignal for 
// further details).
// When the input event in memory already contained calibrated data
// (i.e. de-calibration functions are present in the event data), the event
// data is first de-calibrated (using the corresponding de-calibration functions
// contained in the event data) before the new calibration is performed.
// In case no corresponding calibration function is present, the calibration
// of those specific data will not be performed.
// This implies that running this task on calibrated data without having
// attached an OM database, will result in fully de-calibrated data. 
// In case an OM slot was flagged as bad in the OM database, this flag
// will be copied into the event data for the corresponding OM.
//
// Information about the actual parameter settings can be found in the event
// structure itself via the device named "IceVeto".
//
//--- Author: Nick van Eijndhoven 30-jun-2016 IIHE-VUB, Brussel
//- Modified: NvE $Date$ IIHE-VUB
///////////////////////////////////////////////////////////////////////////
 
#include "IceVeto.h"
#include "Riostream.h"

ClassImp(IceVeto) // Class implementation to enable ROOT I/O

IceVeto::IceVeto(const char* name,const char* title) : TTask(name,title)
{
// Default constructor.

 fVetos=0;
}
///////////////////////////////////////////////////////////////////////////
IceVeto::~IceVeto()
{
// Default destructor.

 if (fVetos)
 {
  delete fVetos;
  fVetos=0;
 }
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::DefineVetoSystem(TString name,Float_t qtot,Float_t amp,Int_t ndom,Int_t nhit,Int_t slc,Float_t tresmin,Float_t tresmax)
{
// Define a veto system.
// The various parameters will be stored in an NcDevice with the specified name.
//
// Input arguments :
// -----------------
// name    : Name given to the veto system c.q. device
// qtot    : Minimal required total signal amplitude in the veto system
// amp     : Minimal single hit amplitude required for a veto hit
// ndom    : Minimal number of different DOMs with a veto hit required to veto an event
// nhit    : Minimal total number of veto hits required to veto an event
// slc     : Flag to allow SLC hits as veto hits (1) or not (0)
// tresmin : Minimal time residual (in ns) required for a veto hit
// tresmax : Maximal time residual (in ns) required for a veto hit
//
// Only hits in the veto system with a time residual within [tresmin,tresmax] will be
// recorded as a veto hit.
//
// Notes :
// -------
// 1) If tresmin>tresmax the time residual is not taken into account. 
// 2) Individual parameters may be modified afterwards by invoking the member function SetVetoParameter(). 


 if (!fVetos)
 {
  fVetos=new TObjArray();
  fVetos->SetOwner();
 }

 Int_t nvetos=fVetos->GetEntries();
 for (Int_t i=0; i<nvetos; i++)
 {
  TObject* obj=fVetos->At(i);
  if (!obj) continue;
  if (name==obj->GetName())
  {
   cout << " *IceVeto::DefineVetoSystem* Name already exists : " << name.Data() << endl;
   cout << " Please specify another (unique) name for the veto system." << endl;
   return;
  }
 }

 if (ndom<=0) ndom=1;
 if (nhit<=0) nhit=1;
 if (slc) slc=1;

 NcVeto* dveto=new NcVeto();
 dveto->SetHitCopy(1);
 dveto->SetNameTitle(name.Data(),"IceVeto system");
 dveto->SetUniqueID(nvetos+1);

 dveto->AddNamedSlot("SLCVeto");
 dveto->AddNamedSlot("AmpVetoMin");
 dveto->AddNamedSlot("NdomVetoMin");
 dveto->AddNamedSlot("NhitVetoMin");
 dveto->AddNamedSlot("QtotVetoMin");
 dveto->AddNamedSlot("TresVetoMin");
 dveto->AddNamedSlot("TresVetoMax");

 dveto->SetSignal(slc,"SLCVeto");
 dveto->SetSignal(amp,"AmpVetoMin");
 dveto->SetSignal(ndom,"NdomVetoMin");
 dveto->SetSignal(nhit,"NhitVetoMin");
 dveto->SetSignal(qtot,"QtotVetoMin");
 dveto->SetSignal(tresmin,"TresVetoMin");
 dveto->SetSignal(tresmax,"TresVetoMax");

 fVetos->Add(dveto);
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::AddVetoDOMs(TString name,Int_t lstring,Int_t ustring,Int_t ldom,Int_t udom)
{
// Specify the veto DOMs to be added to the veto system "name".
// Several veto DOMs can be specified in a single call by the combination
// of a range of string numbers and DOM numbers. 
//
// Input arguments :
// -----------------
// lstring : The lower bound of the string number
// ustring : The upper bound of the string number
// ldom    : The lower bound of the DOM number
// udom    : The upper bound of the DOM number
//
// The DOMs with numbers within [ldom,udom] of the strings with numbers within [lstring,ustring]
// will be registered as veto doms.
//
// Examples :
// ----------
// 1)  lstring=25 ustring=64 ldom=1 udom=8
//     will register the DOMs 1-8 (incl.) of strings 25-64 (incl.) as veto DOMs
// 2)  lstring=38 ustring=38 ldom=4 udom=4
//     will register the single DOM 4 of string 38 as a veto DOM

 if (!fVetos)
 {
  cout << " *IceVeto::AddVetoDOMs* No veto systems have been defined." << endl;
  return;
 }

 NcVeto* dveto=0;
 Int_t ifound=0;
 for (Int_t i=0; i<fVetos->GetEntries(); i++)
 {
  dveto=(NcVeto*)fVetos->At(i);
  if (!dveto) continue;
  if (name==dveto->GetName())
  {
   ifound=1;
   break;
  }
 }

 if (!ifound)
 {
  cout << " *IceVeto::AddVetoDOMs* No veto system found with name : " << name.Data() << endl;
  return;
 }

 Int_t nstrings=ustring-lstring+1;
 Int_t ndoms=udom-ldom+1;
 Int_t nadd=nstrings*ndoms;

 if (nadd<1) return;

 Int_t idom=0;
 NcSignal vdom;
 for (Int_t js=lstring; js<=ustring; js++)
 {
  for (Int_t jd=ldom; jd<=udom; jd++)
  {
   idom=100*abs(js)+jd;
   if (js<0) idom=-idom;

   // Check if this DOM was already registered for this veto system
   NcSignal* sx=dveto->GetIdHit(idom);
   if (!sx)
   {
    vdom.SetUniqueID(idom);
    dveto->AddHit(vdom);
   }
  }
 }

 // Remove "Pre-defined" from the veto system title in case this affected a pre-defined veto system.
 TString title=dveto->GetTitle();
 title.ReplaceAll("Pre-defined ","");
 dveto->SetTitle(title.Data()); 
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::RemoveVetoDOMs(TString name,Int_t lstring,Int_t ustring,Int_t ldom,Int_t udom)
{
// Specify the veto DOMs to be removed from the veto system "name".
// Several veto DOMs can be specified in a single call by the combination
// of a range of string numbers and DOM numbers. 
//
// Input arguments :
// -----------------
// lstring : The lower bound of the string number
// ustring : The upper bound of the string number
// ldom    : The lower bound of the DOM number
// udom    : The upper bound of the DOM number
//
// The DOMs with numbers within [ldom,udom] of the strings with numbers within [lstring,ustring]
// will be removed from the veto system.
//
// Examples :
// ----------
// 1)  lstring=25 ustring=64 ldom=1 udom=8
//     will remove the DOMs 1-8 (incl.) of strings 25-64 (incl.) from the veto system
// 2)  lstring=38 ustring=38 ldom=4 udom=4
//     will remove the single DOM 4 of string 38 from the veto system

 if (!fVetos)
 {
  cout << " *IceVeto::RemoveVetoDOMs* No veto systems have been defined." << endl;
  return;
 }

 NcVeto* dveto=0;
 Int_t ifound=0;
 for (Int_t i=0; i<fVetos->GetEntries(); i++)
 {
  dveto=(NcVeto*)fVetos->At(i);
  if (!dveto) continue;
  if (name==dveto->GetName())
  {
   ifound=1;
   break;
  }
 }

 if (!ifound)
 {
  cout << " *IceVeto::RemoveVetoDOMs* No veto system found with name : " << name.Data() << endl;
  return;
 }

 Int_t nstrings=ustring-lstring+1;
 Int_t ndoms=udom-ldom+1;
 Int_t nrem=nstrings*ndoms;

 if (nrem<1) return;

 Int_t idom=0;
 for (Int_t js=lstring; js<=ustring; js++)
 {
  for (Int_t jd=ldom; jd<=udom; jd++)
  {
   idom=100*abs(js)+jd;
   if (js<0) idom=-idom;

   // Remove the corresponding DOM from this veto system (if present)
   NcSignal* sx=dveto->GetIdHit(idom);
   if (sx) dveto->RemoveHit(sx);
  }
 }

 // Remove "Pre-defined" from the veto system title in case this affected a pre-defined veto system.
 TString title=dveto->GetTitle();
 title.ReplaceAll("Pre-defined ","");
 dveto->SetTitle(title.Data()); 
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::ActivateVetoSystem(TString name,Float_t qtot,Float_t amp,Int_t ndom,Int_t nhit,Int_t slc,Float_t tresmin,Float_t tresmax)
{
// Activate a pre-defined a veto system.
// This facility automatically invokes the corresponding memberfunctions DefineVetoSystem() and AddVetoDOMs()
// for the specified veto system. 
// The various parameters will be stored in an NcDevice with the specified name.
//
// Input arguments :
// -----------------
// name : Name of the pre-defined veto system (see below for available options)
// qtot : Minimal required total signal amplitude in the veto system
// amp  : Minimal single hit amplitude required for a veto hit
// ndom : Minimal number of different DOMs with a veto hit required to veto an event
// nhit : Minimal total number of veto hits required to veto an event
// slc  : Flag to allow SLC hits as veto hits (1) or not (0)
// tresmin : Minimal time residual (in ns) required for a veto hit
// tresmax : Maximal time residual (in ns) required for a veto hit
//
// Only hits in the veto system with a time residual within [tresmin,tresmax] will be
// recorded as a veto hit.
//
// Notes :
// -------
// 1) If tresmin>tresmax the time residual is not taken into account. 
// 2) In case any of the input arguments has a negative value (or tresmin=tresmax), the default value of that parameter
//    will be used according to the specified pre-defined veto system name.
// 3) Individual parameters may be modified afterwards by invoking the member function SetVetoParameter(). 
//
// The default values are : qtot=-1, amp=-1, ndom=-1, nhit=-1, slc=-1, tresmin=0, tresmax=0.
//
// The names of the pre-defined veto systems currently available are :
// -------------------------------------------------------------------
// "IceTop86"     : Simple downgoing charged particle veto using the IceTop tanks
//                  All the IC86 IceTop DOMs are used
// "Upper86"      : Downgoing charged particle veto using the 6 upper IC DOMs 
//                  All the IC DOMs 1-6 (incl.) are used
// "DustLayer86"  : Veto for charged particles sneaking in via the dust layer
//                  All IC DOMs 39-43 (incl.) are used
// "Bottom86"     : Veto for light entering from below produced by downgoing showers missing the detector
//                  All IC bottom DOMs 60 are used 
// "Sides86"      : Veto for charged particles entering from the side
//                  All IC DOMs on the outer strings are used
// "HESE86"       : The veto system that was used for the IC86 HESE events
// "Start86"      : Veto system to select events starting in the IC86 InIce detector
//                  This comprises the veto systems "Upper86", "DustLayer86", "Bottom86" and "Sides86"

 if (name!="IceTop86" && name!="Upper86" && name!="DustLayer86" && name!="Bottom86" && name!="Sides86"
     && name!="HESE86" && name!="Start86")
 {
  cout << " *IceVeto::ActivateVetoSystem* Unknown pre-defined veto system : " << name.Data() << endl;
  return;
 }

 if (qtot<0)
 {
  qtot=0;
  if (name=="HESE86") qtot=3;
 }
 if (amp<0)
 {
  amp=0;
 }
 if (ndom<0)
 {
  ndom=1;
  if (name=="HESE86") ndom=3;
 }
 if (nhit<0)
 {
  nhit=1;
 }
 if (slc<0)
 {
  slc=1;
  if (name=="HESE86" || name=="IceTop86") slc=0;
 }
 if (tresmin==tresmax)
 {
  tresmin=1;
  tresmax=0;
 }

 DefineVetoSystem(name,qtot,amp,ndom,nhit,slc,tresmin,tresmax);

 if (name=="IceTop86")
 {
 // Mark all IceTOP DOMs as veto
  AddVetoDOMs(name,1,86,61,64);
 }

 if (name=="Upper86" || name=="Start86" || name=="HESE86")
 {
  // Mark the top 6 IC DOMs as veto 
  AddVetoDOMs(name,1,79,1,6);
 }

 if (name=="Bottom86" || name=="Start86" || name=="HESE86")
 {
  // Mark the bottom DOM of each IC string as veto 
  AddVetoDOMs(name,1,79,60,60);
 }

 if (name=="DustLayer86" || name=="Start86" || name=="HESE86")
 {
  // Mark all IC DOMs in the dust layer as veto 
  AddVetoDOMs(name,1,79,39,43);
 }

 if (name=="Sides86" || name=="Start86" || name=="HESE86")
 {
  // Mark all DOMs on the IC86 outer strings as veto
  AddVetoDOMs(name,1,7,1,60);
  AddVetoDOMs(name,13,14,1,60);
  AddVetoDOMs(name,21,22,1,60);
  AddVetoDOMs(name,30,31,1,60);
  AddVetoDOMs(name,40,41,1,60);
  AddVetoDOMs(name,50,51,1,60);
  AddVetoDOMs(name,59,60,1,60);
  AddVetoDOMs(name,67,68,1,60);
  AddVetoDOMs(name,72,78,1,60);
 }

 if (name=="HESE86") // Change some configurations for "HESE86" w.r.t. "Start86"
 {
  RemoveVetoDOMs(name,8,8,43,43);  
  RemoveVetoDOMs(name,10,10,43,43);  
  RemoveVetoDOMs(name,11,11,43,43);  
  RemoveVetoDOMs(name,12,12,43,43);  
  RemoveVetoDOMs(name,15,15,60,60);  
  RemoveVetoDOMs(name,16,16,43,43);  
  RemoveVetoDOMs(name,18,18,43,43);  
  RemoveVetoDOMs(name,19,19,43,43);  
  RemoveVetoDOMs(name,20,20,43,43);  
  RemoveVetoDOMs(name,24,24,60,60);  
  RemoveVetoDOMs(name,25,25,60,60);  
  RemoveVetoDOMs(name,26,26,43,43);  
  AddVetoDOMs(name,27,27,38,38);  
  RemoveVetoDOMs(name,27,27,43,43);  
  RemoveVetoDOMs(name,28,28,43,43);  
  RemoveVetoDOMs(name,29,29,60,60);  
  AddVetoDOMs(name,34,34,7,8);  
  RemoveVetoDOMs(name,34,34,39,39);  
  AddVetoDOMs(name,34,34,44,44);  
  RemoveVetoDOMs(name,34,34,60,60);  
  RemoveVetoDOMs(name,35,35,60,60);  
  AddVetoDOMs(name,37,37,7,7);  
  RemoveVetoDOMs(name,37,37,60,60);  
  AddVetoDOMs(name,38,38,38,38);  
  RemoveVetoDOMs(name,38,38,43,43);  
  RemoveVetoDOMs(name,39,39,60,60);  
  RemoveVetoDOMs(name,42,42,60,60);  
  RemoveVetoDOMs(name,45,45,43,43);  
  RemoveVetoDOMs(name,46,46,60,60);  
  RemoveVetoDOMs(name,47,47,60,60);  
  AddVetoDOMs(name,49,49,7,7);  
  RemoveVetoDOMs(name,49,49,60,60);  
  RemoveVetoDOMs(name,52,52,43,43);  
  RemoveVetoDOMs(name,55,55,60,60);  
  RemoveVetoDOMs(name,56,56,60,60);  
  AddVetoDOMs(name,57,57,7,7);  
  RemoveVetoDOMs(name,57,57,60,60);  
  RemoveVetoDOMs(name,58,58,43,43);  
  RemoveVetoDOMs(name,63,63,43,43);  
  AddVetoDOMs(name,64,64,7,8);  
  RemoveVetoDOMs(name,64,64,39,39);  
  AddVetoDOMs(name,64,64,44,44);  
  RemoveVetoDOMs(name,64,64,60,60);  
  AddVetoDOMs(name,65,65,7,7);  
  RemoveVetoDOMs(name,65,65,39,39);  
  RemoveVetoDOMs(name,65,65,60,60);  
  AddVetoDOMs(name,66,66,7,7);  
  RemoveVetoDOMs(name,66,66,39,39);  
  RemoveVetoDOMs(name,66,66,60,60);  
  RemoveVetoDOMs(name,71,71,43,43);  
 }

 // Indicate in the veto system title that this is a pre-defined one.
 TString title="Pre-defined ";
 NcVeto* dveto=GetVetoSystem(name);
 if (dveto)
 {
  title+=dveto->GetTitle();
  dveto->SetTitle(title.Data());
 } 
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::SetVetoParameter(TString sname,TString pname,Double_t pval)
{
// Set c.q. modify a parameter of the specified veto system.
//
// Input arguments :
// -----------------
// sname : Name of the veto system to be modified.
// pname : Name of the parameter to be modified.
// pval  : The new value of the specified parameter.
//
// Supported parameters are :
// --------------------------
// "SLCVeto"     : Flag to allow SLC hits as veto hits (1) or not (0)
// "AmpVetoMin"  : Minimal single hit amplitude required for a veto hit
// "NdomVetoMin" : Minimal number of different DOMs with a veto hit required to veto an event
// "NhitVetoMin" : Minimal total number of veto hits required to veto an event
// "QtotVetoMin" : Minimal required total signal amplitude in the veto system
// "TresVetoMin" : Minimal time residual (in ns) required for a veto hit
// "TresVetoMax" : Maximal time residual (in ns) required for a veto hit
//
// Only hits in the veto system with a time residual within [tresmin,tresmax] will be
// recorded as a veto hit.
//
// Note : If tresmin>tresmax the time residual is not taken into account. 


 if (!fVetos) return;

 if (pname=="NdomVetoMin" && pval<=0) pval=1;
 if (pname=="NhitVetoMin" && pval<=0) pval=1;
 if (pname=="SLCVeto" && pval>0.1) pval=1;

 NcVeto* dveto=0;
 Int_t nvetos=fVetos->GetEntries();
 for (Int_t i=0; i<nvetos; i++)
 {
  TObject* obj=fVetos->At(i);
  if (!obj) continue;
  if (sname != obj->GetName()) continue;

  dveto=(NcVeto*)obj;
  dveto->SetSignal(pval,pname);
  return;
 }
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::Data(Int_t mode)
{
// Provide info on all the registered veto systems.
//
// mode = 0 --> Only the ID, name and number of associated veto DOMs of all the registered veto systems is provided
//        1 --> The same as mode=0 but also the veto system parameters are listed
//        2 --> The same as mode=1 bit also the IDs of all the veto DOMs are listed
//
// Default value : mode=0

 Int_t nvetos=0;
 if (fVetos) nvetos=fVetos->GetEntries();

 cout << " *IceVeto::Data()* Number of registered veto systems : " << nvetos << endl;

 NcVeto* dveto=0;
 TString name;
 TString title;
 Int_t id=0;
 Int_t ndoms=0;
 for (Int_t i=0; i<nvetos; i++)
 {
  dveto=(NcVeto*)fVetos->At(i);
  if (!dveto) continue;
 
  name=dveto->GetName();
  title=dveto->GetTitle();
  id=dveto->GetUniqueID();
  ndoms=dveto->GetNhits();
  cout << " Veto system " << id << " : (" << title.Data() << ") name=" << name.Data() << " nDOMs=" << ndoms << endl;

  if (mode>0) // List also the veto system parameters
  {
   cout << " Parameter settings for this veto system : " << endl;
   dveto->List(-1);
  }

  if (mode==2) // Full DOM listing of the veto system
  {
   cout << " This veto system contains the following DOMs : " << endl;
   dveto->ShowHit();
  }
 }
}
///////////////////////////////////////////////////////////////////////////
NcVeto* IceVeto::GetVetoSystem(TString name)
{
// Provide a pointer to the specified veto system.
//
// In case of inconsistency a value 0 will be returned.

 if (!fVetos) return 0;

 NcVeto* dveto=0;
 for (Int_t i=0; i<fVetos->GetEntries(); i++)
 {
  dveto=(NcVeto*)fVetos->At(i);
  if (!dveto) continue;

  if (name==dveto->GetName()) break;
 }

 return dveto;
}
///////////////////////////////////////////////////////////////////////////
void IceVeto::Exec(Option_t* opt)
{
// Implementation of the (self)vetoing procedure.

 TString name=opt;
 NcJob* parent=(NcJob*)(gROOT->GetListOfTasks()->FindObject(name.Data()));

 if (!parent) return;

 IceEvent* evt=(IceEvent*)parent->GetObject("IceEvent");
 if (!evt) return;

 // Only process accepted events
 NcDevice* seldev=evt->GetDevice("NcEventSelector");
 if (seldev)
 {
  if (seldev->GetSignal("Select") < 0.1) return;
 }

 // The number of all OMs with a signal in the event
 Int_t nmods=evt->GetNdevices("IceGOM");
 if (!nmods) return;

 // Get the light speed in m/ns
 NcAstrolab lab;
 Double_t c=lab.GetPhysicalParameter("SpeedC")*1.e-9;


 Float_t vetolevel=0; // Overall veto level

 // Loop over all the defined veto systems
 NcVeto* dveto=0;
 Float_t lveto=0; // Veto level for a specific system
 Float_t qtotmin=0;
 Float_t ampmin=0;
 Int_t ndommin=0;
 Int_t nhitmin=0;
 Int_t slc=0;
 Float_t tresmin=0;
 Float_t tresmax=0;
 NcSignal* vdom=0;
 NcDevice* omx=0;
 NcPosition rx;
 Double_t tx;
 Double_t dist0=0;
 NcSignal* sx=0;
 Double_t thit=0;
 Double_t tres0=0;
 Int_t domid=0;
 Float_t qtot=0;
 Float_t amp=0;
 Int_t ndom=0;
 Int_t nhit=0;
 Int_t vetohit=0; // Flag to indicate the encounter of a valid veto hit
 TString vetoname;
 NcVeto dum;
 dum.SetHitCopy(0);
 TString dumname;
 TObjArray hits;    // Temp. storage of hits to be analysed
 TObjArray ordered; // Temp. storage of ordered hits to be analysed
 Double_t totamp=0; // Total signal amplitude of the hits to be analysed
 NcPosition r0;     // Reference position (e.g. COG) of the event
 Double_t t0=0;     // Reference time (e.g. central hit time)
 Double_t thres=0;  // Signal threshold to determine the event start time
 Double_t twin=0;   // Time window size to determine the event start time
 Double_t tstart=0; // Start time of the event
 NcPosition rstart; // Position of the start signal of the event
 Double_t dt0=0;
 Double_t dtstart=0;
 Double_t diststart=0;
 Double_t trestart=0;
 Double_t dz0=0;
 Double_t dzstart=0;
 Double_t tresz0=0;
 Double_t treszstart=0;
 Int_t i1,i2; // Indices defining the search window in the time ordered hit array
 for (Int_t isys=0; isys<fVetos->GetEntries(); isys++)
 {
  dveto=(NcVeto*)fVetos->At(isys);
  if (!dveto) continue;

  vetoname=dveto->GetName();

  // Determine the InIce center of gravity, central hit time and event start time
  if (vetoname=="HESE86")
  {
   evt->GetHits("IceICDOM",&hits,"SLC",-2);
  }
  else
  {
///@@@   evt->GetHits("IceIDOM",&hits);
   evt->GetHits("IceIDOM",&hits,"SLC",-2);
  }
  r0=evt->GetCOG(&hits,1,"ADC",8);
  t0=evt->GetCVAL(&hits,"LE","ADC",8);
  totamp=dum.SumSignals("ADC",8,&hits);

  dum.SortHits("LE",1,&hits,8,1,&ordered); // Sort hits with increasing hit time

  thres=0.05*totamp;
  if (thres<3) thres=3;
  twin=3000;
  tstart=dum.SlideWindow(&ordered,thres,twin,"LE",8,"ADC",8,&i1,&i2);

  // Get the position of the event start signal
  rstart.SetZero();
  if (i2>=0)
  {
   sx=(NcSignal*)ordered.At(i2);
   if (sx) omx=sx->GetDevice();
   if (omx) rstart=omx->GetPosition(); 
  }

  // The veto parameters of this veto system
  qtotmin=dveto->GetSignal("QtotVetoMin");
  ampmin=dveto->GetSignal("AmpVetoMin");
  ndommin=dveto->GetSignal("NdomVetoMin");
  nhitmin=dveto->GetSignal("NhitVetoMin");
  slc=dveto->GetSignal("SLCVeto");
  tresmin=dveto->GetSignal("TresVetoMin");
  tresmax=dveto->GetSignal("TresVetoMax");

  dum.Reset();
  dum.SetUniqueID(dveto->GetUniqueID());
  dumname=GetName();
  dumname+="-";
  dumname+=vetoname;
  dum.SetNameTitle(dumname.Data(),dveto->GetTitle());

  evt->AddDevice(dum);
  NcVeto* params=(NcVeto*)evt->GetDevice(dumname);
  if (!params) continue;

  params->AddNamedSlot("SLCVeto");
  params->AddNamedSlot("AmpVetoMin");
  params->AddNamedSlot("NdomVetoMin");
  params->AddNamedSlot("NhitVetoMin");
  params->AddNamedSlot("QtotVetoMin");
  params->AddNamedSlot("TresVetoMin");
  params->AddNamedSlot("TresVetoMax");
  params->AddNamedSlot("NdomVeto");
  params->AddNamedSlot("NhitVeto");
  params->AddNamedSlot("QtotVeto");
  params->AddNamedSlot("VetoLevel");

  lveto=0;
  qtot=0;
  ndom=0;
  nhit=0;

  // Loop over all the veto DOMs of this veto system
  for (Int_t ivdom=1; ivdom<=dveto->GetNhits(); ivdom++)
  {
   vdom=dveto->GetHit(ivdom);
   if (!vdom) continue;

   domid=vdom->GetUniqueID();

   // Check if the corresponding veto DOM fired in the event 
   omx=evt->GetIdDevice(domid,"IceGOM");
   if (!omx) continue;

   rx=omx->GetPosition();
   dist0=rx.GetDistance(r0);
   diststart=rx.GetDistance(rstart);
   dz0=rx.GetX(3,"car")-r0.GetX(3,"car");
   dzstart=rx.GetX(3,"car")-rstart.GetX(3,"car");

   // Loop over all the recorded hits of this fired veto DOM
   vetohit=0;
   for (Int_t ih=1; ih<=omx->GetNhits(); ih++)
   {
    sx=omx->GetHit(ih);
    if (!sx) continue;

    if (!slc && sx->GetSignal("SLC")) continue;

    amp=sx->GetSignal("ADC",8);
    if (amp<ampmin) continue;

    tx=sx->GetSignal("LE",8);
    dt0=tx-t0;
    dtstart=tx-tstart;
    tres0=dt0-(dist0/c);
    trestart=dtstart-(diststart/c);
    tresz0=dt0-(dz0/c);
    treszstart=dt0-(dzstart/c);
    if (tresmin<=tresmax && (tres0<tresmin || tres0>tresmax)) continue;
cout << " @@@ t0:" << t0 << " tstart:" << tstart << " tx:" << tx << " tx-t0:" << dt0 << " tx-tstart:" << dtstart << endl;
cout << " dist0:" << dist0 << " (dist0/c):" << (dist0/c) << " tres0:" << tres0 << endl;
cout << " diststart:" << diststart << " (diststart/c):" << (diststart/c) << " trestart:" << trestart << endl;
cout << " dz0:" << dz0 << " (dz0/c):" << (dz0/c) << " tresz0:" << tresz0 << endl;
cout << " dzstart:" << dzstart << " (dzstart/c):" << (dzstart/c) << " treszstart:" << treszstart << endl;

    // Valid veto hit encountered
    vetohit=1;
    qtot+=amp;
    nhit++; 
    params->AddHit(sx);
   } // End of loop over the hits of this veto DOM

   if (vetohit) ndom++;   
  } // End of loop over the veto DOMs of this system
  
  if (qtot>=qtotmin && ndom>=ndommin && nhit>=nhitmin)
  {
   lveto=1;
   vetolevel+=1;
  }

  // Add values of observables and veto level to the parameters of this veto system
  params->SetSignal(slc,"SLCVeto");
  params->SetSignal(ampmin,"AmpVetoMin");
  params->SetSignal(ndommin,"NdomVetoMin");
  params->SetSignal(nhitmin,"NhitVetoMin");
  params->SetSignal(qtotmin,"QtotVetoMin");
  params->SetSignal(tresmin,"TresVetoMin");
  params->SetSignal(tresmax,"TresVetoMax");
  params->SetSignal(ndom,"NdomVeto");
  params->SetSignal(nhit,"NhitVeto");
  params->SetSignal(qtot,"QtotVeto");
  params->SetSignal(lveto,"VetoLevel");
 } // End of loop over the various veto systems

 // Enter the final overall veto level into the event structure
 dum.StoreVetoLevel(evt,vetolevel);
}
///////////////////////////////////////////////////////////////////////////
