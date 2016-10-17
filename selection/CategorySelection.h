//CategorySelection.h

#ifndef ADD_CATEGORYSELECTION
#define ADD_CATEGORYSELECTION

#include "VarSet.h"
#include "JetSelectionTools.h"
#include "TH1F.h"
#include "TList.h"
#include <map>
#include <iostream>

class Category
{
   public:
       Category(){};
       ~Category(){};

       Category(TString name)
       {
           this->name = name;
           this->hide = false;
       }
    
       Category(TString name, bool hide)
       {
           this->name = name;
           this->hide = hide;
       }
    
       // Categorizer.evaluate will determine whether an event falls into this category
       // if an event falls into this category the boolean value will be set to true
       bool inCategory = false; 

       // make a histogram of the category or not
       bool hide = false;

       // Usually we want to make a bunch of histograms for each category, so we keep them here
       std::map<TString, TH1F*> histoMap;   // access histos by name
       TList* histoList = new TList();      // histos ordered by xsec so that we can create the ratio and stack plot
       TList* signalList = new TList();     // signal histos
       TList* bkgList = new TList();        // bkg histos
       TList* dataList = new TList();       // data histo

       // Book-keeping
       TString name;
};


class Categorizer
{
// This class evaluates the event and determines which category or categories it belongs to

    public:
        // the categories the event may fall into
        std::map<TString, Category> categoryMap;

        // set up the categories and map them to a tstring
        virtual void initCategoryMap() = 0;

        // Determine which category the event belongs to
        virtual void evaluate(VarSet& vars) = 0;

        // reset the boolean values for the categories
        void reset()
        {
            for(auto &entry : categoryMap)
                entry.second.inCategory = false;
        };

        // output the category selection results
        void outputResults()
        {
            for(auto &entry : categoryMap)
                std::cout << "    " << entry.first << ": " << entry.second.inCategory << std::endl;

            std::cout << std::endl;
        };
};

class CategorySelectionRun1 : public Categorizer
{
// This is based off of the run1 H->MuMu category selection
    public:
        CategorySelectionRun1(); 
        CategorySelectionRun1(float cLeadPtMin, float cSubleadPtMin, float cMETMax, float cDijetMassMinVBFT, float cDijetDeltaEtaMinVBFT, float cDijetMassMinGGFT,
                              float cDimuPtMinGGFT, float cDimuPtMin01T); 

        // Preselection
        float cLeadPtMin;
        float cSubleadPtMin;
        float cMETMax;

        // VBF Tight
        float cDijetMassMinVBFT;
        float cDijetDeltaEtaMinVBFT;
     
        // GGF Tight
        float cDijetMassMinGGFT;
        float cDimuPtMinGGFT;

        // 01Tight
        float cDimuPtMin01T; 

        // Determine which category the event belongs to
        // result stored in isVBFTight, isGGFTight, etc 
        void evaluate(VarSet& vars);
        void initCategoryMap();
};

class CategorySelectionFEWZ : public Categorizer
{
// Categories to compare DY, Data, and DY-FEWZ

    public:
        CategorySelectionFEWZ(); 
        CategorySelectionFEWZ(bool useRecoMu, bool useRecoJets); 
        CategorySelectionFEWZ(bool useReco, bool useRecoJets, float massSplit, float etaCentralSplit, float jetPtMin, float jetEtaMax); 

        bool useRecoMu;
        bool useRecoJets;

        // Selections
        float cMassSplit;
        float cEtaCentralSplit;
        float cJetPtMin;
        float cJetEtaMax;

        // Determine which category the event belongs to
        void evaluate(VarSet& vars);
        void initCategoryMap();
};
#endif
