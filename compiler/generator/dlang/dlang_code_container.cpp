/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2017 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#include "dlang_code_container.hh"
#include "Text.hh"
#include "exception.hh"
#include "fir_function_builder.hh"
#include "floats.hh"
#include "global.hh"

using namespace std;

/*
 DLang backend description:

 - 'usize' type has to be used for all array access: cast index as 'usize' only when using it (load/store arrays)
 - TODO: local stack variables (shared computation) are normally non-mutable
 - inputN/outputN local buffer variables in 'compute' are not created at all: they are replaced directly in the code
 with inputs[N]/outputs[N] (done in instructions_compiler.cpp)
 - BoolOpcode BinOps always casted to integer
 - 'delete' for SubcContainers is not generated
 - add  'fDummy' zone in 'mydsp' struct to publish it (if needed) in 'declare' when nullptr is used
 - add 'kMutable' and 'kReference' address access type

*/

map<string, bool> DLangInstVisitor::gFunctionSymbolTable;

dsp_factory_base* DLangCodeContainer::produceFactory()
{
    return new text_dsp_factory_aux(
        fKlassName, "", "",
        ((dynamic_cast<ostringstream*>(fOut)) ? dynamic_cast<ostringstream*>(fOut)->str() : ""), "");
}

CodeContainer* DLangCodeContainer::createScalarContainer(const string& name, int sub_container_type)
{
    return new DLangScalarCodeContainer(name, 0, 1, fOut, sub_container_type);
}

CodeContainer* DLangCodeContainer::createContainer(const string& name, int numInputs, int numOutputs, ostream* dst)
{
    gGlobal->gDSPStruct = true;
    CodeContainer* container;

    if (gGlobal->gMemoryManager) {
        throw faustexception("ERROR : -mem not supported for DLang\n");
    }
    if (gGlobal->gFloatSize == 3) {
        throw faustexception("ERROR : quad format not supported for DLang\n");
    }
    if (gGlobal->gOpenCLSwitch) {
        throw faustexception("ERROR : OpenCL not supported for DLang\n");
    }
    if (gGlobal->gCUDASwitch) {
        throw faustexception("ERROR : CUDA not supported for DLang\n");
    }

    if (gGlobal->gOpenMPSwitch) {
        // container = new DLangOpenMPCodeContainer(name, numInputs, numOutputs, dst);
        throw faustexception("ERROR : OpenMP not supported for DLang\n");
    } else if (gGlobal->gSchedulerSwitch) {
        // container = new DLangWorkStealingCodeContainer(name, numInputs, numOutputs, dst);
        throw faustexception("ERROR : Scheduler not supported for DLang\n");
    } else if (gGlobal->gVectorSwitch) {
        // container = new DLangVectorCodeContainer(name, numInputs, numOutputs, dst);
        throw faustexception("ERROR : Vector not supported for DLang\n");
    } else {
        container = new DLangScalarCodeContainer(name, numInputs, numOutputs, dst, kInt);
    }

    return container;
}

void DLangCodeContainer::produceInternal()
{
    int n = 0;

    // Global declarations
    tab(n, *fOut);
    fCodeProducer.Tab(n);
    generateGlobalDeclarations(&fCodeProducer);

    tab(n, *fOut);
    *fOut << "pub struct " << fKlassName << " {";

    tab(n + 1, *fOut);

    // Fields
    fCodeProducer.Tab(n + 1);
    generateDeclarations(&fCodeProducer);

    back(1, *fOut);
    *fOut << "}";

    tab(n, *fOut);
    tab(n, *fOut);
    *fOut << "impl " << fKlassName << " {";

    tab(n + 1, *fOut);
    tab(n + 1, *fOut);
    produceInfoFunctions(n + 1, fKlassName, "&self", false, false, &fCodeProducer);

    // Init
    // TODO
    // generateInstanceInitFun("instanceInit" + fKlassName, false, false)->accept(&fCodeProducer);

    tab(n + 1, *fOut);
    *fOut << "fn instance_init" << fKlassName << "(&mut self, sample_rate: i32) {";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateInit(&fCodeProducer);
    generateResetUserInterface(&fCodeProducer);
    generateClear(&fCodeProducer);
    back(1, *fOut);
    *fOut << "}";

    // Fill
    tab(n + 1, *fOut);
    string counter = "count";
    if (fSubContainerType == kInt) {
        tab(n + 1, *fOut);
        *fOut << "fn fill" << fKlassName << subst("(&mut self, $0: i32, table: &mut[i32]) {", counter);
    } else {
        tab(n + 1, *fOut);
        *fOut << "fn fill" << fKlassName << subst("(&mut self, $0: i32, table: &mut[$1]) {", counter, ifloat());
    }
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateComputeBlock(&fCodeProducer);
    SimpleForLoopInst* loop = fCurLoop->generateSimpleScalarLoop(counter);
    loop->accept(&fCodeProducer);
    back(1, *fOut);
    *fOut << "}" << endl;

    tab(n, *fOut);
    *fOut << "}" << endl;

    // Memory methods
    tab(n, *fOut);
    tab(n, *fOut);
    *fOut << "pub fn new" << fKlassName << "() -> " << fKlassName << " { ";
    tab(n + 1, *fOut);
    *fOut << fKlassName << " {";
    DLangInitFieldsVisitor initializer(fOut, n + 2);
    generateDeclarations(&initializer);
    tab(n + 1, *fOut);
    *fOut << "}";
    tab(n, *fOut);
    *fOut << "}";
}

void DLangCodeContainer::produceClass()
{
    int n = 0;

    // Sub containers
    generateSubContainers();

    // Functions
    tab(n, *fOut);
    fCodeProducer.Tab(n);
    generateGlobalDeclarations(&fCodeProducer);

    *fOut << "pub struct " << fKlassName << " {";
    tab(n + 1, *fOut);

    // Dummy field used for 'declare'
    *fOut << "fDummy: " << ifloat() << ",";
    tab(n + 1, *fOut);

    // Fields
    fCodeProducer.Tab(n + 1);
    generateDeclarations(&fCodeProducer);

    back(1, *fOut);
    *fOut << "}";
    tab(n, *fOut);

    tab(n, *fOut);
    *fOut << "impl FaustDsp for " << fKlassName << " {";

    // Associated type
    tab(n + 1, *fOut);
    *fOut << "type Sample = " << ifloat() << ";";

    // Memory methods
    tab(n + 2, *fOut);
    if (fAllocateInstructions->fCode.size() > 0) {
        tab(n + 2, *fOut);
        *fOut << "static void allocate" << fKlassName << "(" << fKlassName << "* dsp) {";
        tab(n + 2, *fOut);
        fAllocateInstructions->accept(&fCodeProducer);
        back(1, *fOut);
        *fOut << "}";
    }

    tab(n + 1, *fOut);

    if (fDestroyInstructions->fCode.size() > 0) {
        tab(n + 1, *fOut);
        *fOut << "static void destroy" << fKlassName << "(" << fKlassName << "* dsp) {";
        tab(n + 2, *fOut);
        fDestroyInstructions->accept(&fCodeProducer);
         back(1, *fOut);
        *fOut << "}";
        tab(n + 1, *fOut);
    }

    *fOut << "fn new() -> " << fKlassName << " { ";
    if (fAllocateInstructions->fCode.size() > 0) {
        tab(n + 2, *fOut);
        *fOut << "allocate" << fKlassName << "(dsp);";
    }
    tab(n + 2, *fOut);
    *fOut << fKlassName << " {";
    tab(n + 3, *fOut);
    *fOut << "fDummy: 0 as " << ifloat() << ",";
    DLangInitFieldsVisitor initializer(fOut, n + 3);
    generateDeclarations(&initializer);
    tab(n + 2, *fOut);
    *fOut << "}";
    tab(n + 1, *fOut);
    *fOut << "}";

    // Print metadata declaration
    produceMetadata(n + 1);

    // Get sample rate method
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);
    generateGetSampleRate("get_sample_rate", "&self", false, false)->accept(&fCodeProducer);

    produceInfoFunctions(n + 1, "", "&self", false, false, &fCodeProducer);

    // Inits

    // TODO
    //
    // CInstVisitor codeproducer1(fOut, "");
    // codeproducer1.Tab(n+2);
    // generateStaticInitFun("classInit" + fKlassName, false)->accept(&codeproducer1);
    // generateInstanceInitFun("instanceInit" + fKlassName, false, false)->accept(&codeproducer2);

    tab(n + 1, *fOut);
    *fOut << "fn class_init(sample_rate: i32) {";
    {
        tab(n + 2, *fOut);
        // Local visitor here to avoid DSP object type wrong generation
        DLangInstVisitor codeproducer(fOut, "");
        codeproducer.Tab(n + 2);
        // TODO: This creates function calls with "wrong" function names. How to forward the proper names?
        generateStaticInit(&codeproducer);
    }
    back(1, *fOut);
    *fOut << "}";

    tab(n + 1, *fOut);
    *fOut << "fn instance_reset_user_interface(&mut self) {";
    {
        tab(n + 2, *fOut);
        // Local visitor here to avoid DSP object type wrong generation
        DLangInstVisitor codeproducer(fOut, "");
        codeproducer.Tab(n + 2);
        generateResetUserInterface(&codeproducer);
    }
    back(1, *fOut);
    *fOut << "}";

    tab(n + 1, *fOut);
    *fOut << "fn instance_clear(&mut self) {";
    {
        tab(n + 2, *fOut);
        // Local visitor here to avoid DSP object type wrong generation
        DLangInstVisitor codeproducer(fOut, "");
        codeproducer.Tab(n + 2);
        generateClear(&codeproducer);
    }
    back(1, *fOut);
    *fOut << "}";

    tab(n + 1, *fOut);
    *fOut << "fn instance_constants(&mut self, sample_rate: i32) {";
    {
        tab(n + 2, *fOut);
        // Local visitor here to avoid DSP object type wrong generation
        DLangInstVisitor codeproducer(fOut, "");
        codeproducer.Tab(n + 2);
        generateInit(&codeproducer);
    }
    back(1, *fOut);
    *fOut << "}";

    tab(n + 1, *fOut);
    *fOut << "fn instance_init(&mut self, sample_rate: i32) {";
    tab(n + 2, *fOut);
    *fOut << "self.instance_constants(sample_rate);";
    tab(n + 2, *fOut);
    *fOut << "self.instance_reset_user_interface();";
    tab(n + 2, *fOut);
    *fOut << "self.instance_clear();";
    tab(n + 1, *fOut);
    *fOut << "}";

    tab(n + 1, *fOut);
    *fOut << "fn init(&mut self, sample_rate: i32) {";
    tab(n + 2, *fOut);
    *fOut << fKlassName << "::class_init(sample_rate);";
    tab(n + 2, *fOut);
    *fOut << "self.instance_init(sample_rate);";
    tab(n + 1, *fOut);
    *fOut << "}";

    // User interface
    tab(n + 1, *fOut);
    *fOut << "fn build_user_interface(&mut self, ui_interface: &mut dyn UI<Self::Sample>) {";
    tab(n + 2, *fOut);
    fCodeProducer.Tab(n + 2);
    generateUserInterface(&fCodeProducer);
    back(1, *fOut);
    *fOut << "}";

    // Compute
    generateCompute(n + 1);

    tab(n, *fOut);
    *fOut << "}" << endl;
    tab(n, *fOut);
}

void DLangCodeContainer::produceMetadata(int n)
{
    tab(n, *fOut);
    *fOut << "fn metadata(&self, m: &mut dyn Meta) { ";

    // We do not want to accumulate metadata from all hierachical levels, so the upper level only is kept
    for (auto& i : gGlobal->gMetaDataSet) {
        if (i.first != tree("author")) {
            tab(n + 1, *fOut);
            *fOut << "m.declare(\"" << *(i.first) << "\", " << **(i.second.begin()) << ");";
        } else {
            // But the "author" meta data is accumulated, the upper level becomes the main author and sub-levels become
            // "contributor"
            for (set<Tree>::iterator j = i.second.begin(); j != i.second.end(); j++) {
                if (j == i.second.begin()) {
                    tab(n + 1, *fOut);
                    *fOut << "m.declare(\"" << *(i.first) << "\", " << **j << ");";
                } else {
                    tab(n + 1, *fOut);
                    *fOut << "m.declare(\""
                          << "contributor"
                          << "\", " << **j << ");";
                }
            }
        }
    }

    tab(n, *fOut);
    *fOut << "}" << endl;
}

void DLangCodeContainer::produceInfoFunctions(int tabs, const string& classname, const string& obj, bool ismethod, bool isvirtual,
                                             TextInstVisitor* producer)
{
    producer->Tab(tabs);
    generateGetInputs(subst("get_num_inputs$0", classname), obj, false, false)->accept(&fCodeProducer);
    generateGetOutputs(subst("get_num_outputs$0", classname), obj, false, false)->accept(&fCodeProducer);
    producer->Tab(tabs);
    generateGetInputRate(subst("get_input_rate$0", classname), obj, false, false)->accept(&fCodeProducer);
    producer->Tab(tabs);
    generateGetOutputRate(subst("get_output_rate$0", classname), obj, false, false)->accept(&fCodeProducer);
}

// Scalar
DLangScalarCodeContainer::DLangScalarCodeContainer(const string& name, int numInputs, int numOutputs, std::ostream* out,
                                                 int sub_container_type)
    : DLangCodeContainer(name, numInputs, numOutputs, out)
{
    fSubContainerType = sub_container_type;
}

void DLangScalarCodeContainer::generateCompute(int n)
{
    // Generates declaration
    tab(n, *fOut);
    tab(n, *fOut);
    *fOut << "fn compute("
          << subst("&mut self, $0: i32, inputs: &[&[Self::Sample]], outputs: &mut[&mut[Self::Sample]]) {", fFullCount);
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);

    // Generates local variables declaration and setup
    generateComputeBlock(&fCodeProducer);

    // Generates one single scalar loop
    SimpleForLoopInst* loop = fCurLoop->generateSimpleScalarLoop(fFullCount);
    loop->accept(&fCodeProducer);

    back(1, *fOut);
    *fOut << "}" << endl;
}

// Vector
DLangVectorCodeContainer::DLangVectorCodeContainer(const string& name, int numInputs, int numOutputs, std::ostream* out)
    : VectorCodeContainer(numInputs, numOutputs), DLangCodeContainer(name, numInputs, numOutputs, out)
{
}

void DLangVectorCodeContainer::generateCompute(int n)
{
    // Possibly generate separated functions
    fCodeProducer.Tab(n);
    tab(n, *fOut);
    generateComputeFunctions(&fCodeProducer);

    // Compute declaration
    tab(n, *fOut);
    *fOut << "fn compute("
          << subst("&mut self, $0: i32, inputs: &[&[Self::Sample]], outputs: &mut[&mut[Self::Sample]]) {", fFullCount);
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);

    // Generates local variables declaration and setup
    generateComputeBlock(&fCodeProducer);

    // Generates the DSP loop
    fDAGBlock->accept(&fCodeProducer);

    back(1, *fOut);
    *fOut << "}" << endl;
}

// OpenMP
DLangOpenMPCodeContainer::DLangOpenMPCodeContainer(const string& name, int numInputs, int numOutputs, std::ostream* out)
    : OpenMPCodeContainer(numInputs, numOutputs), DLangCodeContainer(name, numInputs, numOutputs, out)
{
}

void DLangOpenMPCodeContainer::generateCompute(int n)
{
    // Possibly generate separated functions
    fCodeProducer.Tab(n);
    tab(n, *fOut);
    generateComputeFunctions(&fCodeProducer);

    // Compute declaration
    tab(n, *fOut);
    *fOut << "fn compute("
          << subst("&mut self, $0: i32, inputs: &[&[Self::Sample]], outputs: &mut[&mut[Self::Sample]]) {", fFullCount);
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);

    // Generates local variables declaration and setup
    generateComputeBlock(&fCodeProducer);

    // Generate it
    fGlobalLoopBlock->accept(&fCodeProducer);

    back(1, *fOut);
    *fOut << "}" << endl;
}

// Works stealing scheduler
DLangWorkStealingCodeContainer::DLangWorkStealingCodeContainer(const string& name, int numInputs, int numOutputs,
                                                             std::ostream* out)
    : WSSCodeContainer(numInputs, numOutputs, "dsp"), DLangCodeContainer(name, numInputs, numOutputs, out)
{
}

void DLangWorkStealingCodeContainer::generateCompute(int n)
{
    // Possibly generate separated functions
    fCodeProducer.Tab(n);
    tab(n, *fOut);
    generateComputeFunctions(&fCodeProducer);

    // Generates "computeThread" code
    // Note that users either have to adjust the trait in their architecture file.
    // Alternatively we would have to attach this method to the impl, not the trait.
    tab(n, *fOut);
    *fOut << "pub fn compute_thread(" << fKlassName << "&mut self, num_thread: i32) {";
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);

    // Generate it
    fThreadLoopBlock->accept(&fCodeProducer);

    tab(n, *fOut);
    *fOut << "}" << endl;

    // Compute "compute" declaration
    tab(n, *fOut);
    *fOut << "fn compute("
          << subst("&mut self, $0: i32, inputs: &[&[Self::Sample]], outputs: &mut[&mut[Self::Sample]]) {", fFullCount);
    tab(n + 1, *fOut);
    fCodeProducer.Tab(n + 1);

    // Generates local variables declaration and setup
    generateComputeBlock(&fCodeProducer);

    tab(n, *fOut);
    *fOut << "}" << endl;

    tab(n, *fOut);
    *fOut << "extern \"C\" void computeThreadExternal(&mut self, num_thread: i32) {";
    tab(n + 1, *fOut);
    *fOut << "compute_thread((" << fKlassName << "*)dsp, num_thread);";
    tab(n, *fOut);
    *fOut << "}" << endl;
}
