#ifndef IceVeto_h
#define IceVeto_h

// Copyright(c) 2003, IceCube Experiment at the South Pole, All rights reserved.
// See cxx source for full Copyright notice.

// $Id$

#include "TTask.h"

#include "NcVeto.h"
#include "IceEvent.h"
#include "IceGOM.h"
#include "NcJob.h"
#include "NcAstrolab.h"

class IceVeto : public TTask
{
 public :
  IceVeto(const char* name="IceVeto",const char* title="IceCube event vetoing procedures"); // Constructor
  virtual ~IceVeto();                                   // Destructor
  virtual void Exec(Option_t* opt);                     // Perform the (self)vetoing
  void DefineVetoSystem(TString name,Float_t qtot,Float_t amp,Int_t ndom,Int_t nhit,Int_t slc,Float_t tresmin,Float_t tresmax);  // Define a veto system
  void AddVetoDOMs(TString name,Int_t lstring,Int_t ustring,Int_t ldom,Int_t udom); // Specify the veto doms to be added to the veto system "name"
  void RemoveVetoDOMs(TString name,Int_t lstring,Int_t ustring,Int_t ldom,Int_t udom); // Specify the veto doms to be removed from the veto system "name"
  void ActivateVetoSystem(TString name,Float_t qtot=-1,Float_t amp=-1,Int_t ndom=-1,Int_t nhit=-1,Int_t slc=-1,Float_t tresmin=0,Float_t tresmax=0);  // Activate a pre-defined veto system
  void SetVetoParameter(TString sname,TString pname,Double_t pval); // Set c.q. modify a parameter of the specified veto system.
  void Data(Int_t mode=0);                              // Provide info on the registered veto procedures
  NcVeto* GetVetoSystem(TString name);                  // Provide a pointer to the specified veto system

 protected :
  TObjArray* fVetos;   // Array with devices that contain the various veto definitions

 ClassDef(IceVeto,1) // TTask derived class to perform (self)vetoing of events
};
#endif
