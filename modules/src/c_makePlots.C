#include <boost/python.hpp>
#include "boost/python/extract.hpp"
#include "boost/python/numeric.hpp"
#include "boost/python/list.hpp"
#include "boost/python/str.hpp"
//#include "boost/filesystem.hpp"
#include <iostream>
#include <stdint.h>
#include "TString.h"
#include <string>
#include <boost/python/exception_translator.hpp>
#include <exception>
#include "../interface/pythonToSTL.h"
#include "friendTreeInjector.h"
#include "TROOT.h"
#include "colorToTColor.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TFile.h"
#include "TStyle.h"

using namespace boost::python; //for some reason....



void makePlots(
        const boost::python::list intextfiles,
        const boost::python::list names,
        const boost::python::list variables,
        const boost::python::list cuts,
        const boost::python::list colors,
        std::string outfile,
        std::string xaxis,
        std::string yaxis,
        bool normalized) {


    std::vector<TString>  s_intextfiles=toSTLVector<TString>(intextfiles);
    std::vector<TString>  s_vars = toSTLVector<TString>(variables);
    std::vector<TString>  s_names = toSTLVector<TString>(names);
    std::vector<TString>  s_colors = toSTLVector<TString>(colors);
    std::vector<TString>  s_cuts = toSTLVector<TString>(cuts);

    TString toutfile=outfile;
    if(!toutfile.EndsWith(".pdf"))
        throw std::runtime_error("makePlots: output files need to be pdf format");


    if(!s_names.size())
        throw std::runtime_error("makePlots: needs at least one legend entry");
    /*
     * Size checks!!!
     */
    if(s_intextfiles.size() !=s_names.size()||
            s_names.size() != s_vars.size() ||
            s_names.size() != s_colors.size()||
            s_names.size() != s_cuts.size())
        throw std::runtime_error("makePlots: input lists must have same size");

    //make unique list of infiles
    std::vector<TString> u_infiles;
    std::vector<TString> aliases;
    TString oneinfile="";
    bool onlyonefile=true;
    for(const auto& f:s_intextfiles){
        if(oneinfile.Length()<1)
            oneinfile=f;
        else
            if(f!=oneinfile)
                onlyonefile=false;
    }
    for(const auto& f:s_intextfiles){
        //if(std::find(u_infiles.begin(),u_infiles.end(),f) == u_infiles.end()){
        u_infiles.push_back(f);
        TString s="";
        s+=aliases.size();
        aliases.push_back(s);
        //	std::cout << s <<std::endl;
        //}
    }



    friendTreeInjector injector;
    for(size_t i=0;i<u_infiles.size();i++){
        if(!aliases.size())
            injector.addFromFile((TString)u_infiles.at(i));
        else
            injector.addFromFile((TString)u_infiles.at(i),aliases.at(i));
    }
    injector.createChain();

    TChain* c=injector.getChain();
    std::vector<TH1F*> allhistos;
    TLegend * leg=new TLegend(0.2,0.75,0.8,0.88);
    leg->SetBorderSize(0);

    leg->SetNColumns(3);
    leg->SetFillStyle(0);

    TString addstr="";
    if(normalized)
        addstr="normalized";
    float max=-1e100;
    float min=1e100;

    TString tfileout=toutfile;
    tfileout=tfileout(0,tfileout.Length()-4);
    tfileout+=".root";

    TFile * f = new TFile(tfileout,"RECREATE");
    gStyle->SetOptStat(0);

    for(size_t i=0;i<s_names.size();i++){
        TString tmpname="hist_";
        tmpname+=i;
        c->Draw(s_vars.at(i)+">>"+tmpname,s_cuts.at(i),addstr);
        TH1F *histo = (TH1F*) gROOT->FindObject(tmpname);
        histo->SetLineColor(colorToTColor(s_colors.at(i)));
        histo->SetLineStyle(lineToTLineStyle(s_colors.at(i)));
        histo->SetTitle(s_names.at(i));
        histo->SetName(s_names.at(i));

        histo->SetFillStyle(0);
        histo->SetLineWidth(2);

        float tmax=histo->GetMaximum();
        float tmin=histo->GetMinimum();
        if(tmax>max)max=tmax;
        if(tmin<min)min=tmin;
        allhistos.push_back(histo);

        histo->Write();

        leg->AddEntry(histo,s_names.at(i),"l");
    }

    TCanvas cv("plots");

    allhistos.at(0)->Draw("AXIS");
    allhistos.at(0)->GetYaxis()->SetRangeUser(min,1.3*max); //space for legend on top

    allhistos.at(0)->GetXaxis()->SetTitle(xaxis.data());
    allhistos.at(0)->GetYaxis()->SetTitle(yaxis.data());

    allhistos.at(0)->Draw("AXIS");
    for(size_t i=0;i<s_names.size();i++){
        allhistos.at(i)->Draw("same,hist");
    }
    leg->Draw("same");

    cv.Write();
    cv.Print(toutfile);

    f->Close();

}




// Expose classes and methods to Python
BOOST_PYTHON_MODULE(c_makePlots) {
    //__hidden::indata();//for some reason exposing the class prevents segfaults. garbage collector?
    //anyway, it doesn't hurt, just leave this here
    def("makePlots", &makePlots);

}
