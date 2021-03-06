///
/// \file CreateEMCAL_OADB_RunByRunTemperatureECalibCorrection.C
/// \ingroup EMCAL_OADB
/// \brief Create OADB file with calibration Temperature corrections
///
/// Example to Create or Read OADB Container 
/// for EMCal Run by Run calibration dependent 
/// on Temperature variations
///
/// Input files and information can be found in
/// https://twiki.cern.ch/twiki/bin/viewauth/ALICE/EMCalTimeDependentCalibrations
///
/// \author Gustavo Conesa Balbastre, <Gustavo.Conesa.Balbastre@cern.ch>, LPSC-CNRS ???
///

#if !defined(__CINT__)
#include <TH1S.h>
#include <TSystem.h>

#include <Riostream.h>

#include "AliEMCALGeometry.h"
#include "AliEMCALCalibTimeDepCorrection.h"
#include "AliOADBContainer.h"
#endif

///< rescale some 2011 runs slightly (0.5%):
const Float_t rescaleFactor = 1.005;

const Int_t kFirst = 156620; ///< First run in range
const Int_t kLast  = 162740; ///< Last run in range

///
/// Create OADB container for Temperature calibration parameters
///
/// Get the list of run numbers to be added to the OADB, parameters provided usually in a 
/// root file per run
/// Tar ball with all the files can be found here
/// https://twiki.cern.ch/twiki/bin/viewauth/ALICE/EMCalTimeDependentCalibrations
///
void Create()
{
  gSystem->Load("libOADB");            
  
  AliOADBContainer* con = new AliOADBContainer("AliEMCALRunDepTempCalibCorrections");
  
  ifstream fList;
  fList.open("CorrectionFiles/runlist.txt");
  
  Int_t runNumber  = 0;
  Float_t multiplier = 10000; // allows us to store the values in TH1S with some precision
  TString string;
  Int_t nRuns=0;
  Int_t nSM = 12;
  
  AliEMCALGeometry* geom = AliEMCALGeometry::GetInstance("EMCAL_COMPLETE12SMV1");
  
  if (fList.good()) 
  {
    while( string.ReadLine(fList, kFALSE) ) 
    {
      sscanf(string.Data(), "%d",&runNumber);
      
      if     (runNumber < 140000) nSM = 4;
      else if(runNumber < 200000) nSM = 10;
      
      if(runNumber>100000)
      {
        multiplier = 10000; // allows us to store the values in TH1S with some precision
        if (runNumber>=kFirst && runNumber<=kLast) 
        { // rescale some runs
          multiplier *= rescaleFactor;
        }
        
        printf("Run %d multiplier %5.0f\n", runNumber, multiplier);
        
        // Access class that contains methods to read the content of 
        // the calibration file per run
        AliEMCALCalibTimeDepCorrection  *corr =  new AliEMCALCalibTimeDepCorrection();
        corr->ReadRootInfo(Form("CorrectionFiles/Run%d_Correction.root",runNumber));
        
        // Init the histogram
        TH1S *h = new TH1S(Form("h%d",runNumber),"",24*48*nSM,0,24*48*nSM);
        
        for(Int_t ism = 0; ism < nSM; ism++) {
          for(Int_t icol = 0; icol < 48; icol++) {
            for(Int_t irow = 0; irow < 24; irow++) {
              Float_t recalFactor = corr->GetCorrection(ism, icol,irow,0);            
              Int_t absID = geom->GetAbsCellIdFromCellIndexes(ism, irow, icol);
              
              h->SetBinContent(absID,(Short_t)(recalFactor*multiplier));
            }
          }
        }
        
        con->AddDefaultObject(h);
        
        //Establishing run number with the correct objects
        con->AppendObject(h,runNumber,runNumber);
        
        delete corr;
        
        nRuns++;
      }
    }
  }
  
  fList.close();
  printf(" *** nRuns ***  %d\n",nRuns);
  
  // add dummy object at the end of file:
  runNumber++;
  multiplier = 10000;
  // Init the histogram
  printf("Dummy/extra Run %d at EOF multiplier %5.0f\n", runNumber, multiplier);  
  TH1S *h = new TH1S(Form("h%d",runNumber),"",24*48*nSM,0,24*48*nSM);
  
  for(Int_t ism = 0; ism < nSM; ism++) {
    for(Int_t icol = 0; icol < 48; icol++) {
      for(Int_t irow = 0; irow < 24; irow++) {
        Float_t recalFactor = 1;
        Int_t absID = geom->GetAbsCellIdFromCellIndexes(ism, irow, icol);
        
        h->SetBinContent(absID,(Short_t)(recalFactor*multiplier));
      }
    }
  }
  
  con->AddDefaultObject(h);    
  //Establishing run number with the correct objects
  con->AppendObject(h,runNumber,runNumber);
  
  con->WriteToFile("EMCALTemperatureCorrCalib.root");   
  
}

///
/// Read OADB parameters for
/// \param runNumber: reference run number
///
void Read(Int_t runNumber = 170387)
{
  gSystem->Load("libOADB");            
  
  AliOADBContainer *cont=new AliOADBContainer("");
  cont->InitFromFile("$ALICE_ROOT/OADB/EMCAL/EMCALTemperatureCorrCalib.root", "AliEMCALRunDepTempCalibCorrections");
  
  //cout<<"_________--------------- dump ---------------------___________"<<endl;
  //cont->Dump();
  
  //cout<<"cont->GetDefaultList()->Print()"<<endl;
  //cont->GetDefaultList()->Print();
  
  TH1S *h = (TH1S*) cont->GetObject(runNumber); //GetObject(int runnumber)
  
  if (h) 
  {
    printf("runNumber %d found\n", runNumber);
  }
  else 
  {
    printf("runNumber %d not found\n", runNumber);
    // let's get the closest runnumber
    Int_t lower = 0;
    Int_t ic = 0;
    Int_t maxEntry = cont->GetNumberOfEntries();
    
    while ( (ic < maxEntry) && (cont->UpperLimit(ic) < runNumber) ) {
      lower = ic;
      ic++; 
    }
    
    Int_t closest = lower;
    if ( (ic<maxEntry) && 
        (cont->LowerLimit(ic)-runNumber) < (runNumber - cont->UpperLimit(lower)) ) {
      closest = ic;
    }
    
    cout << "found closest id " << closest 
    << " from run " << cont->LowerLimit(closest) << endl;
    h = (TH1S*) cont->GetObjectByIndex(closest); 
    h->Print(); // tmp debug
    cout << endl;
  }
  
  AliEMCALGeometry* geom = AliEMCALGeometry::GetInstance("EMCAL_COMPLETE12SMV1");
  
  // Read parameter file line-by-line  
  // Get number of lines first
  
  Int_t nSM = 10;
  
  for(Int_t iabsID = 0; iabsID < 24*48*nSM; iabsID++)
  {
    printf("absID %d, content %1.1f\n",iabsID,h->GetBinContent(iabsID));
  }
  
  h->Draw();
  
}

/*******************************************************************
 *  NOTE: Sorting function to sort the final OADB file             *
 *                  by ascending runnumber                         *
 *******************************************************************/
void Sort(const char *fileNameOADB=""){

    TFile *f                                    = TFile::Open(fileNameOADB);
    f->ls();
    AliOADBContainer *con                       =(AliOADBContainer*)f->Get("AliEMCALRunDepTempCalibCorrections");
    con->SetName("Old"); 

    Int_t indexAdd                              = 0;
    Int_t largerthan                            = 0;
    Int_t currentvalue                          = 0;

    AliOADBContainer *con2                      = new AliOADBContainer("AliEMCALRunDepTempCalibCorrections");
    // First entry needs to be added before sorting loop
    //con2->AddDefaultObject(con->GetObjectByIndex(0));
    con2->AppendObject(con->GetObjectByIndex(0),con->LowerLimit(0),con->UpperLimit(0));
    TString strTemp                             = "";

    // sorting magic happens here
    for(int i=1;i<con->GetNumberOfEntries();i++){
        largerthan                              = con2->UpperLimit(con2->GetNumberOfEntries()-1);
        currentvalue                            = -1;
        indexAdd                                = 0;
        for(int j=0;j<con->GetNumberOfEntries();j++){
            if(con->UpperLimit(j)<=largerthan) 
                continue;
            if(currentvalue < 0){
                currentvalue                    = con->UpperLimit(j);
                indexAdd                        = j;
            }
            if(con->UpperLimit(j)<currentvalue){
                currentvalue                    = con->UpperLimit(j);
                indexAdd                        = j;
            }
        }
        //con2->AddDefaultObject(con->GetObjectByIndex(indexAdd));
        con2->AppendObject(con->GetObjectByIndex(indexAdd),con->LowerLimit(indexAdd),con->UpperLimit(indexAdd));
    }

    printf("\n\n");
    Int_t nentries2                             = con2->GetNumberOfEntries();
    for(int i=0;i<nentries2;i++){
        printf("\n Entry2 --> %d/%d -->",i,nentries2);
        printf("%d -- %d --> obj = %p , %s", con2->LowerLimit(i),con2->UpperLimit(i),con2->GetObjectByIndex(i),con2->GetObjectByIndex(i)->GetName());
    }
    printf("\n\n");

    con2->WriteToFile("EMCALTemperatureCorrCalibNEW.root");
}

///
/// Main method
///
/// \param opt: 0 just read; 1 create file
/// \param runNumber: if opt=1, read content for this run
///
void CreateEMCAL_OADB_RunByRunTemperatureECalibCorrection(Int_t opt = 2, Int_t runNumber = 170387)
{
// const char *fileNameOADB                ="$ALICE_DATA/OADB/EMCAL/EMCALTemperatureCorrCalib.root";
const char *fileNameOADB                ="EMCALTemperatureCorrCalib.root";
  if(opt == 0) Read(runNumber);
  if(opt == 1) Create();
  //Sort(fileNameOADB);
//   rebuildContainer("EMCALTemperatureCorrCalibNEW.root");
  rebuildContainer("EMCALTemperatureCorrCalib.root");
}


TObjArray *CreatePeriodContainer(TObjArray *inputcont){
  TObjArray *newcont = new TObjArray(inputcont->GetEntries());
  newcont->SetName(inputcont->GetName());
  for(int i = 0; i < inputcont->GetEntries(); i++){
    newcont->AddAt(inputcont->At(i)->Clone(), i);
  }
  return newcont;
}

/*******************************************************************
 *  NOTE: Function required to fix OADB ownership                  *
 *                                                                 *
 *******************************************************************/
void rebuildContainer(const char *fileNameOADB=""){
  TFile *reader = TFile::Open(fileNameOADB);
  AliOADBContainer *cont = static_cast<AliOADBContainer *>(reader->Get("AliEMCALRunDepTempCalibCorrections"));
  delete reader;

  AliOADBContainer *newcont = new AliOADBContainer("AliEMCALRunDepTempCalibCorrections");
  for(int irun = 0; irun < cont->GetNumberOfEntries(); irun++){
//     newcont->AppendObject(CreatePeriodContainer(static_cast<TObjArray *>(cont->GetObjArray()->At(irun))), cont->LowerLimit(irun), cont->UpperLimit(irun));
    newcont->AppendObject(static_cast<TObjArray *>(cont->GetObjArray()->At(irun)), cont->LowerLimit(irun), cont->UpperLimit(irun));
  }

  newcont->WriteToFile("EMCALTemperatureCorrCalibOADBfix.root");

  TFile *reader = TFile::Open("EMCALTemperatureCorrCalibOADBfix.root", "READ");
    AliOADBContainer *cont = static_cast<AliOADBContainer *>(reader->Get("AliEMCALRunDepTempCalibCorrections"));
    delete reader;
    delete cont;
}
