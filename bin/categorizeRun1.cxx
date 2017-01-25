// plot different variables in the run 1 categories.
// may be used to look for discrepancies between data and mc or to make dimu_mass plots for limit setting.
// outputs mc stacks with data overlayed and a ratio plot underneath.
// also saves the histos needed to make the mc stack, data.
// Also saves net BKG histo and net signal histo for limit setting.

// Missing HLT trigger info in CMSSW_8_0_X MC so we have to compare Data and MC in a different manner.
// We apply triggers to data but not to MC. Then scale MC for trigger efficiency.

#include "Sample.h"
#include "DiMuPlottingSystem.h"
#include "SelectionCuts.h"
#include "CategorySelection.h"
#include "JetSelectionTools.h"
#include "MuonSelectionTools.h"
#include "ElectronSelectionTools.h"
#include "TauSelectionTools.h"

#include "EventTools.h"
#include "PUTools.h"

#include "SignificanceMetrics.hxx"
#include "SampleDatabase.cxx"
#include "ThreadPool.hxx"

#include <sstream>
#include <map>
#include <vector>
#include <utility>

#include "TLorentzVector.h"
#include "TSystem.h"

//////////////////////////////////////////////////////////////////
//---------------------------------------------------------------
//////////////////////////////////////////////////////////////////

UInt_t getNumCPUs()
{
  SysInfo_t s;
  gSystem->GetSysInfo(&s);
  UInt_t ncpu  = s.fCpus;
  return ncpu;
}

//////////////////////////////////////////////////////////////////
//---------------------------------------------------------------
//////////////////////////////////////////////////////////////////

// set bins, min, max, and the var to plot based upon terminal input: input and binning
void initPlotSettings(int input, int binning, int& bins, float& min, float& max, TString& varname)
{
    // dimu_mass
    if(input == 0)
    {
        if(binning == 0)
        {
            bins = 150;
            min = 50;
            max = 200;
        }

        else if(binning == 1)
        {
            bins = 50;
            min = 110;
            max = 160;
        }
        else if(binning == 2)
        {
            bins = 100;
            min = 110;
            max = 310;
        }
        else
        {
            bins = 150;
            min = 50;
            max = 200;
        }

        varname = "dimu_mass";
    }

    // dimu_pt 
    if(input == 1)
    {
        bins = 200;
        min = 0;
        max = 100;
        varname = "dimu_pt";
    }

    // mu_pt
    if(input == 2)
    {
        bins = 200;
        min = 0;
        max = 150;
        varname = "mu_pt";
    }
 
    // mu_eta
    if(input == 3)
    {
        bins = 100;
        min = -2.5;
        max = 2.5;
        varname = "mu_eta";
    }

    // NPV
    if(input == 4)
    {
        bins = 50;
        min = 0;
        max = 50;
        varname = "NPV";
    }

    // jet_pt
    if(input == 5)
    {   
        bins = 200;
        min = 0;
        max = 200;
        varname = "jet_pt";
    }   

    // jet_eta 
    if(input == 6)
    {   
        bins = 100;
        min = -5; 
        max = 5;
        varname = "jet_eta";
    }   

    // N_valid_jets
    if(input == 7)
    {   
        bins = 11;
        min = 0; 
        max = 11;
        varname = "N_valid_jets";
    }   

    // m_jj
    if(input == 8)
    {   
        bins = 200;
        min = 0; 
        max = 2000;
        varname = "m_jj";
    }   

    // dEta_jj
    if(input == 9)
    {   
        bins = 100;
        min = -10; 
        max = 10;
        varname = "dEta_jj";
    }   
}


//////////////////////////////////////////////////////////////////
//---------------------------------------------------------------
//////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{

    // save the errors for the histogram correctly so they depend upon 
    // the number used to fill originally rather than the scaling
    TH1::SetDefaultSumw2();

    int nthreads = 10;   // number of threads to use in parallelization
    int input = 0;       // the variable to plot, 0 is dimu_mass for instance
    bool rebin = false;  // rebin the histograms so that the ratio plots have small errors
    int binning = 0;     // binning = 1 -> plot dimu_mass from 110 to 160 for limit setting
    bool fitratio = 0;   // fit the ratio plot (data/mc) under the stack w/ a straight line

    for(int i=1; i<argc; i++)
    {   
        std::stringstream ss; 
        ss << argv[i];
        if(i==1) ss >> input;
        if(i==2) ss >> nthreads;
        if(i==3) ss >> rebin;
        if(i==4) ss >> binning;
        if(i==5) ss >> fitratio;
    }   
    std::cout << "@@@ nCPUs Available: " << getNumCPUs() << std::endl;
    std::cout << "@@@ nCPUs used     : " << nthreads << std::endl;

    // Not sure that we need a map if we have a vector
    // Should use this as the main database and choose from it to make the vector
    std::map<std::string, Sample*> samples;

    // Second container so that we can have a copy sorted by cross section.
    std::vector<Sample*> samplevec;

    // Use this to plot some things if we wish
    DiMuPlottingSystem* dps = new DiMuPlottingSystem();

    //float luminosity = 3990;     // pb-1
    float luminosity = 33598;      // pb-1
    //float luminosity = 27217;    // pb-1
    float triggerSF = 0.913;       // no HLT trigger info available for the samples so we scale for the trigger efficiency instead
    float signalSF = 100;          // not using this at the moment, but scale the signal samples to see them better in the plots if you want
    float reductionFactor = 1;     // reduce the number of events you run over in case you want to debug or some such thing

    ///////////////////////////////////////////////////////////////////
    // SAMPLES---------------------------------------------------------
    ///////////////////////////////////////////////////////////////////

    // gather samples map from SamplesDatabase.cxx
    getSamples(luminosity, samples);

    ///////////////////////////////////////////////////////////////////
    // PREPROCESSING---------------------------------------------------
    ///////////////////////////////////////////////////////////////////

    // Loop through all of the samples to do some pre-processing
    std::cout << std::endl;
    std::cout << "======== Preprocess the samples... " << std::endl;
    std::cout << std::endl;

    //makePUHistos(samples);
    
    for(auto &i : samples)
    {
        // Output some info about the current file
        std::cout << "  /// Preprocessing " << i.second->name << std::endl;
        std::cout << std::endl;
        std::cout << "    sample name:       " << i.second->name << std::endl;
        std::cout << "    sample file:       " << i.second->filename << std::endl;
        std::cout << "    pileup file:       " << i.second->pileupfile << std::endl;
        std::cout << "    nOriginal:         " << i.second->nOriginal << std::endl;
        std::cout << "    N:                 " << i.second->N << std::endl;
        std::cout << "    nOriginalWeighted: " << i.second->nOriginalWeighted << std::endl;
        std::cout << std::endl;

        if(!i.second->sampleType.EqualTo("data"))
        {
            // Pileup reweighting
            std::cout << "    +++ PU Reweighting " << i.second->name << "..."  << std::endl;
            std::cout << std::endl;

            // Every data sample has the pileup hist for the entire lumi
            i.second->lumiWeights = new reweight::LumiReWeighting(i.second->pileupfile.Data(), samples["RunB"]->pileupfile.Data(), "pileup", "pileup");
            std::cout << "        " << i.first << "->lumiWeights: " << i.second->lumiWeights << std::endl;
            std::cout << std::endl;
        }
        samplevec.push_back(i.second);
    }

    // Sort the samples by xsec. Useful when making the histogram stack.
    std::sort(samplevec.begin(), samplevec.end(), [](Sample* a, Sample* b){ return a->xsec < b->xsec; }); 

    ///////////////////////////////////////////////////////////////////
    // Get Histo Information from Input -------------------------------
    ///////////////////////////////////////////////////////////////////

    TString varname;
    int bins;
    float min;
    float max;

    // set nbins, min, max, and var to plot based upon terminal inputs: input and binning
    initPlotSettings(input, binning, bins, min, max, varname);

    std::cout << std::endl;
    std::cout << "======== Plot Configs ========" << std::endl;
    std::cout << "var         : " << varname << std::endl;
    std::cout << "min         : " << min << std::endl;
    std::cout << "max         : " << max << std::endl;
    std::cout << "bins        : " << bins << std::endl;
    std::cout << "rebin       : " << rebin << std::endl;
    std::cout << std::endl;

    ///////////////////////////////////////////////////////////////////
    // Define Task for Parallelization -------------------------------
    ///////////////////////////////////////////////////////////////////

    auto makeHistoForSample = [varname, bins, min, max, rebin, triggerSF, luminosity, reductionFactor](Sample* s)
    {
      // Output some info about the current file
      std::cout << std::endl;
      std::cout << "  /// Processing " << s->name << std::endl;

      ///////////////////////////////////////////////////////////////////
      // INIT Cuts and Categories ---------------------------------------
      ///////////////////////////////////////////////////////////////////
      
      // Objects to help with the cuts and selections
      JetSelectionTools jetSelectionTools;
      CategorySelectionRun1 categorySelection;
      Run1MuonSelectionCuts run1MuonSelection;
      Run1EventSelectionCuts80X run1EventSelectionData(true);
      Run1EventSelectionCuts80X run1EventSelectionMC;

      bool isData = s->sampleType.EqualTo("data");

      ///////////////////////////////////////////////////////////////////
      // INIT HISTOGRAMS TO FILL ----------------------------------------
      ///////////////////////////////////////////////////////////////////

      // Fewer bins for lowstats categories if necessary
      int lowstatsbins = bins;
      if(!rebin)  lowstatsbins = bins/5;

      // If we are dealing with NPV or N_valid_jets then don't change the binning
      if(varname.Contains("N")) lowstatsbins = bins;

      // Keep track of which histogram to fill in the category
      TString hkey = s->name;

      // Different categories for the analysis
      for(auto &c : categorySelection.categoryMap)
      {
          //number of bins for the histogram
          int hbins;

          // c.second is the category object, c.first is the category name
          TString hname = c.first+"_"+s->name;

          // The VBF categories have low stats so we use fewer bins
          // Same goes for 01jet categories with dijet variables
          if(c.first.Contains("VBF") || c.first.Contains("GGF") || (c.first.Contains("01_Jet") && varname.Contains("jj"))) hbins = lowstatsbins; 
          else hbins = bins;

          // Set up the histogram for the category and variable to plot
          c.second.histoMap[hkey] = new TH1D(hname, hname, hbins, min, max);
          c.second.histoMap[hkey]->GetXaxis()->SetTitle(varname);
          c.second.histoList->Add(c.second.histoMap[hkey]);                                        // need them ordered by xsec for the stack and ratio plot
          if(s->sampleType.EqualTo("data")) c.second.dataList->Add(c.second.histoMap[hkey]);      // data histo
          if(s->sampleType.EqualTo("signal")) c.second.signalList->Add(c.second.histoMap[hkey]);  // signal histos
          if(s->sampleType.EqualTo("background")) c.second.bkgList->Add(c.second.histoMap[hkey]); // bkg histos

      }


      // Link only to the branches we need to save a lot of time
      TBranch* dimuCandBranch  = s->tree->GetBranch("dimuCand");
      TBranch* recoMuonsBranch = s->tree->GetBranch("recoMuons");
      TBranch* pfJetsBranch    = s->tree->GetBranch("pfJets");
      TBranch* metBranch       = s->tree->GetBranch("met");
      TBranch* eventInfoBranch = s->tree->GetBranch("eventInfo");

      dimuCandBranch->SetAddress(&s->vars.dimuCand);
      recoMuonsBranch->SetAddress(&s->vars.recoMuons);
      pfJetsBranch->SetAddress(&s->vars.jets);
      metBranch->SetAddress(&s->vars.met);
      eventInfoBranch->SetAddress(&s->vars.eventInfo);

      TBranch* nPUBranch = 0;
      TBranch* genWeightBranch = 0;

      if(!isData)
      { 
          nPUBranch       = s->tree->GetBranch("nPU");
          genWeightBranch = s->tree->GetBranch("genWeight");
          genWeightBranch->SetAddress(&s->vars.genWeight);
          nPUBranch->SetAddress(&s->vars.nPU);
      }


      ///////////////////////////////////////////////////////////////////
      // LOOP OVER EVENTS -----------------------------------------------
      ///////////////////////////////////////////////////////////////////

      for(unsigned int i=0; i<s->N/reductionFactor; i++)
      {

        // only load essential information for the first set of cuts 
        dimuCandBranch->GetEntry(i);
        recoMuonsBranch->GetEntry(i);

        ///////////////////////////////////////////////////////////////////
        // CUTS  ----------------------------------------------------------
        ///////////////////////////////////////////////////////////////////


        if(!run1EventSelectionData.evaluate(s->vars) && isData)
        { 
            continue; 
        }
        if(!run1EventSelectionMC.evaluate(s->vars) && !isData)
        { 
            continue; 
        }
        if(!s->vars.recoMuons.isTightMuon[0] || !s->vars.recoMuons.isTightMuon[1])
        { 
            continue; 
        }
        if(!run1MuonSelection.evaluate(s->vars)) 
        {
            continue; 
        }


        // Load the rest of the information needed for run1 categories
        pfJetsBranch->GetEntry(i);
        metBranch->GetEntry(i);
        //eventInfoBranch->GetEntry(i);

        if(!isData)
        {
            genWeightBranch->GetEntry(i);
            nPUBranch->GetEntry(i);
        }

        s->vars.validJets = std::vector<TLorentzVector>();
        jetSelectionTools.getValidJetsdR(s->vars, s->vars.validJets);
        //std::pair<int,int> e(s->vars.eventInfo.run, s->vars.eventInfo.event); // create a pair that identifies the event uniquely


        // Figure out which category the event belongs to
        categorySelection.evaluate(s->vars);


        // Look at each category
        for(auto &c : categorySelection.categoryMap)
        {
            // skip categories
            if(c.second.hide) continue;
            if(!c.second.inCategory) continue;

            // dimuCand.recoCandMass
            if(varname.EqualTo("dimu_mass")) 
            {
                float varvalue = s->vars.dimuCand.recoCandMassPF;
                // blind the signal region for data but not for MC
                if(!(isData && varvalue >= 110 && varvalue < 140))
                {
                    // if the event is in the current category then fill the category's histogram for the given sample and variable
                    c.second.histoMap[hkey]->Fill(varvalue, s->getWeight());
                    //std::cout << "    " << c.first << ": " << varvalue;
                }
               continue;
            }
            // dimuCand.recoCandMass

            if(varname.EqualTo("dimu_pt"))
            {
                // if the event is in the current category then fill the category's histogram for the given sample and variable
                c.second.histoMap[hkey]->Fill(s->vars.dimuCand.recoCandPtPF, s->getWeight());
                continue;
            }

            if(varname.EqualTo("mu_pt"))
            {
                c.second.histoMap[hkey]->Fill(s->vars.recoMuons.pt[0], s->getWeight());
                c.second.histoMap[hkey]->Fill(s->vars.recoMuons.pt[1], s->getWeight());
                continue;
            }

            // recoMu_Eta
            if(varname.EqualTo("mu_eta"))
            {
                c.second.histoMap[hkey]->Fill(s->vars.recoMuons.eta[0], s->getWeight());
                c.second.histoMap[hkey]->Fill(s->vars.recoMuons.eta[1], s->getWeight());
                continue;
            }

            // NPV
            if(varname.EqualTo("NPV"))
            {
                 c.second.histoMap[hkey]->Fill(s->vars.vertices.nVertices, s->getWeight());
                 continue;
            }

            // jet_pt
            if(varname.EqualTo("jet_pt"))
            {
                 for(unsigned int j=0; j<s->vars.validJets.size(); j++)
                     c.second.histoMap[hkey]->Fill(s->vars.validJets[j].Pt(), s->getWeight());
                 continue;
            }

            // jet_eta
            if(varname.EqualTo("jet_eta"))
            {
                 for(unsigned int j=0; j<s->vars.validJets.size(); j++)
                     c.second.histoMap[hkey]->Fill(s->vars.validJets[j].Eta(), s->getWeight());
                 continue;
            }

            // N_valid_jets
            if(varname.EqualTo("N_valid_jets"))
            {
                 c.second.histoMap[hkey]->Fill(s->vars.validJets.size(), s->getWeight());
                 continue;
            }

            // m_jj
            if(varname.EqualTo("m_jj"))
            {
                 if(s->vars.validJets.size() >= 2)
                 {
                     TLorentzVector dijet = s->vars.validJets[0] + s->vars.validJets[1];
                     c.second.histoMap[hkey]->Fill(dijet.M(), s->getWeight());
                 }
                 continue;
            }

            // dEta_jj
            if(varname.EqualTo("dEta_jj"))
            {
                 if(s->vars.validJets.size() >= 2)
                 {
                     float dEta = s->vars.validJets[0].Eta() - s->vars.validJets[1].Eta();
                     c.second.histoMap[hkey]->Fill(dEta, s->getWeight());
                 }
                 continue;
            }

        } // end category loop

        if(false)
          // ouput pt, mass info etc for the event
          EventTools::outputEvent(s->vars, categorySelection);

        // Reset the flags in preparation for the next event
        categorySelection.reset();

      } // end event loop

      // Scale according to luminosity and sample xsec now that the histograms are done being filled for that sample
      for(auto &c : categorySelection.categoryMap)
      {
          c.second.histoMap[hkey]->Scale(s->getScaleFactor(luminosity));
          if(!isData) c.second.histoMap[hkey]->Scale(triggerSF);
      }

      return categorySelection;

    }; // done defining makeHistoForSample

   ///////////////////////////////////////////////////////////////////
   // LOOP OVER EVENTS -----------------------------------------------
   ///////////////////////////////////////////////////////////////////


    ThreadPool pool(nthreads);
    std::vector< std::future<CategorySelectionRun1> > results;

    TStopwatch timerWatch;
    timerWatch.Start();

    for(auto &s : samplevec)
        results.push_back(pool.enqueue(makeHistoForSample, s));


    for(auto && categorySelection: results)
    {
        for(auto& category: categorySelection.get().categoryMap)
        {
            if(category.second.hide) continue;
            for(auto& c: category.second.histoMap)
            {
                std::cout << c.second->GetName() << ", " << c.second->Integral() << std::endl;
            }
        }
    }

    timerWatch.Stop();
    std::cout << "### DONE " << timerWatch.RealTime() << " seconds" << std::endl;

/*
    TList* varstacklist = new TList();   // list to save all of the stacks
    TList* signallist = new TList();     // list to save all of the signal histos
    TList* bglist = new TList();         // list to save all of the background histos
    TList* datalist = new TList();       // list to save all of the data histos
    TList* netlist = new TList();        // list to save all of the net histos

    for(auto &c : categorySelection.categoryMap)
    {
        // some categories are intermediate and we don't want to save the plots for those
        if(c.second.hide) continue;

        // Create the stack and ratio plot    
        TString cname = c.first+"_stack";
        //stackedHistogramsAndRatio(TList* list, TString name, TString title, TString xaxistitle, TString yaxistitle, bool rebin = false, bool fit = true,
                                  //TString ratiotitle = "Data/MC", bool log = true, bool stats = false, int legend = 0);
        // stack signal, bkg, and data
        TCanvas* stack = dps->stackedHistogramsAndRatio(c.second.histoList, cname, cname, varname, "Num Entries", rebin);
        varstacklist->Add(stack);

        // lists will contain signal, bg, and data histos for every category
        signallist->Add(c.second.signalList);
        bglist->Add(c.second.bkgList);
        datalist->Add(c.second.dataList);
       
        // we need the data histo, the net signal, and the net bkg dimu mass histos for the datacards
        // so we make these histos. Might as well make them for every variable, not just dimu_mass.
        TH1D* hNetSignal = dps->addHists(c.second.signalList, c.first+"_Net_Signal", c.first+"_Net_Signal");
        TH1D* hNetBkg    = dps->addHists(c.second.bkgList,    c.first+"_Net_Bkg",    c.first+"_Net_Bkg");
        TH1D* hNetData   = dps->addHists(c.second.dataList,   c.first+"_Net_Data",   c.first+"_Net_Data");

        netlist->Add(hNetSignal);
        netlist->Add(hNetBkg);
        netlist->Add(hNetData);

        stack->SaveAs("imgs/"+cname+".png");
    }
    std::cout << std::endl;

    TString savename = "validate_"+varname+Form("_%d_%d", (int)min, (int)max)+
                       "_x69p2_8_0_X_MC_run1categories_"+Form("%d",(int)luminosity)+Form("_rebin%d.root", (int)rebin);

    std::cout << "  /// Saving plots to " << savename << " ..." << std::endl;
    std::cout << std::endl;
    TFile* savefile = new TFile(savename, "RECREATE");

    TDirectory* stacks        = savefile->mkdir("stacks");
    TDirectory* signal_histos = savefile->mkdir("signal_histos");
    TDirectory* bg_histos     = savefile->mkdir("bg_histos");
    TDirectory* data_histos   = savefile->mkdir("data_histos");
    TDirectory* net_histos    = savefile->mkdir("net_histos");

    // save the different histos and stacks in the appropriate directories in the tfile
    stacks->cd();
    varstacklist->Write();

    signal_histos->cd();
    signallist->Write();

    bg_histos->cd();
    bglist->Write();

    data_histos->cd();
    datalist->Write();

    net_histos->cd();
    netlist->Write();

    savefile->Close();
 
    std::cout << "&&& Elapsed Time: " << timerWatch.RealTime() << std::endl;
*/
    return 0;
}
