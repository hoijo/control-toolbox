/***********************************************************************************
Copyright (c) 2017, Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo,
Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#ifndef INCLUDE_CT_CORE_FUNCTION_DERIVATIVESCPPAD_CG_H_
#define INCLUDE_CT_CORE_FUNCTION_DERIVATIVESCPPAD_CG_H_

#include <ct/core/templateDir.h>
#include "CppadUtils.h"
#include "DerivativesCppadSettings.h"

namespace ct {
namespace core {

//! Jacobian using Auto-Diff Codegeneration
/*!
 * Uses Auto-Diff code generation to compute the Jacobian \f$ J(x_s) = \frac{df}{dx} |_{x=x_s} \f$ of
 * a regular vector-valued mathematical function \f$ y = f(x) \f$ .
 *
 * x has IN_DIM dimension and y has OUT_DIM dimension. Thus, they can be
 * scalar functions (IN_DIM = 1, OUT_DIM = 1), fixed or variable size
 * (IN_DIM = -1, OUT_DIM = -1) functions.
 *
 * \note In fact, this class is called Jacobian but computes also zero order derivatives
 *
 * @tparam IN_DIM Input dimensionality of the function (use Eigen::Dynamic (-1) for dynamic size)
 * @tparam OUT_DIM Output dimensionailty of the function (use Eigen::Dynamic (-1) for dynamic size)
 */
template <int IN_DIM, int OUT_DIM>
class DerivativesCppadCG : public CppadUtils<IN_DIM, OUT_DIM> 
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    typedef ADCGScalar CG_SCALAR; //!< CG_SCALAR  type

    typedef Eigen::Matrix<CG_SCALAR, IN_DIM, 1> IN_TYPE_CG; //!< function input vector type
    typedef Eigen::Matrix<CG_SCALAR, OUT_DIM, 1> OUT_TYPE_CG; //!< function  output vector type
    typedef Eigen::Matrix<bool, IN_DIM, OUT_DIM> Sparsity; //!< Sparsity pattern type
    typedef Eigen::Matrix<bool, IN_DIM, IN_DIM> HessianSparsity;        

    typedef std::function<OUT_TYPE_CG(const IN_TYPE_CG&)> FUN_TYPE_CG; //!< function type

    typedef CppadUtils<IN_DIM, OUT_DIM> Utils;


    /**
     * @brief      Contructs the derivatives for codegeneration using a
     *             FUN_TYPE_CG function
     * @warning    If IN_DIM and/our OUT_DIM are set to dynamic (-1), then the
     *             actual dimensions of x and y have to be passed here.
     *
     * @param      f          The function to be autodiffed
     * @param[in]  inputDim   inputDim input dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     * @param[in]  outputDim  outputDim output dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     */
    DerivativesCppadCG(
        const DerivativesCppadSettings settings, 
        FUN_TYPE_CG& f, 
        int inputDim = IN_DIM, 
        int outputDim = OUT_DIM) 
    :
        Utils(f, inputDim, outputDim),
        settings_(settings)
    {
        if(outputDim > 0 && inputDim > 0)
            this->recordCg();
    }

    //! copy constructor
    DerivativesCppadCG(const DerivativesCppadCG& arg) 
    :
        Utils(arg),
        settings_(arg.settings_),
        tmpVarCount_(arg.tmpVarCount_)
    {}

    //! destructor
    virtual ~DerivativesCppadCG() {}

    //! deep cloning of Jacobian
    DerivativesCppadCG* clone() const override{
        return new DerivativesCppadCG<IN_DIM, OUT_DIM>(*this);
    }

    //! Generates code for computing the Jacobian and writes it to file
    /*!
     * This function is optional and can be used if you want to generate code for a Jacobian
     * and write it to file. This can be useful to pre-compile Jacobians rather than compiling
     * them at runtime using compileJIT(). This function can make use of a template file in which
     * the keyword "AUTOGENERATED_CODE_PLACEHOLDER" is replaced with the autogenerated code.
     *
     * @param derivativeName name of the resulting Jacobian class
     * @param outputDir output directory
     * @param templateDir directory in which template file is located
     * @param ns1 first namespace layer of the Jacobian class
     * @param ns2 second namespace layer of the Jacobian class
     * @param sparsity sparsity pattern to generate sparse Jacobian
     * @param useReverse if true, uses Auto-Diff reverse mode, otherwise uses forward mode
     * @param ignoreZero do not assign 0 to sparse entries or zero entries
     */
    void generateJacobianSource(
            const std::string& derivativeName,
            const std::string& outputDir = ct::core::CODEGEN_OUTPUT_DIR,
            const std::string& templateDir = ct::core::CODEGEN_TEMPLATE_DIR,
            const std::string& ns1 = "core",
            const std::string& ns2 = "generated",
            const Sparsity& sparsity = Sparsity::Ones(),
            bool useReverse = true,
            bool ignoreZero = true)
    {
        internal::SparsityPattern pattern;
        pattern.initPattern(sparsity);

        size_t jacDimension = IN_DIM * OUT_DIM;

        std::string codeJac =
                internal::CGHelpers::generateJacobianSource(
                        this->fCgCppad_,
                        pattern,
                        jacDimension,
                        tmpVarCount_,
                        useReverse,
                        ignoreZero
                );

        writeCodeFile(templateDir, "/Jacobian.tpl.h", "/Jacobian.tpl.cpp", outputDir, derivativeName, ns1, ns2, codeJac, "AUTOGENERATED_CODE_PLACEHOLDER");
    }

    //! Generates code for computing the zero-order derivative and writes it to file
    /*!
     * This function is optional and can be used if you want to generate code for a zero-order derivative,
     * i.e. the function itself and write it to file. While it seems weird at first to regenerate code from
     * existing code, this can help to speed up complex computations. Generating source code can be useful
     * to pre-compile the zero order dynamics rather than compiling them at runtime using compileJIT().
     * This function can make use of a template file in which the keyword "AUTOGENERATED_CODE_PLACEHOLDER"
     * is replaced with the autogenerated code.
     *
     * @param forwardZeroName name of the resulting class
     * @param outputDir output directory
     * @param templateDir directory in which template file is located
     * @param ns1 first namespace layer of the class
     * @param ns2 second namespace layer of the class
     * @param ignoreZero do not assign 0 to sparse entries or zero entries
     */
    void generateForwardZeroSource(
            const std::string& forwardZeroName,
            const std::string& outputDir = ct::core::CODEGEN_OUTPUT_DIR,
            const std::string& templateDir = ct::core::CODEGEN_TEMPLATE_DIR,
            const std::string& ns1 = "core",
            const std::string& ns2 = "generated",
            bool ignoreZero = true)
    {
        std::string codeJac =
                internal::CGHelpers::generateForwardZeroSource(
                        this->fCgCppad_,
                        tmpVarCount_,
                        ignoreZero
                );

        writeCodeFile(templateDir, "/ForwardZero.tpl.h", "/ForwardZero.tpl.cpp", outputDir, forwardZeroName, ns1, ns2, codeJac, "AUTOGENERATED_CODE_PLACEHOLDER");
    }

    //! Generates code for computing the Hessian and writes it to file
    /*!
     * This function is optional and can be used if you want to generate code for a hessian,
     * i.e. the function itself and write it to file. While it seems weird at first to regenerate code from
     * existing code, this can help to speed up complex computations. Generating source code can be useful
     * to pre-compile the hessian dynamics rather than compiling them at runtime using compileJIT().
     * This function can make use of a template file in which the keyword "AUTOGENERATED_CODE_PLACEHOLDER"
     * is replaced with the autogenerated code.
     *
     * @param derivativeName name of the resulting class
     * @param outputDir output directory
     * @param templateDir directory in which template file is located
     * @param ns1 first namespace layer of the class
     * @param ns2 second namespace layer of the class
     * @param sparsity sparsity pattern to generate sparse Jacobian
     * @param useReverse if true, uses Auto-Diff reverse mode, otherwise uses forward mode
     * @param ignoreZero do not assign 0 to sparse entries or zero entries
     */
    void generateHessianSource(
            const std::string& derivativeName,
            const std::string& outputDir = ct::core::CODEGEN_OUTPUT_DIR,
            const std::string& templateDir = ct::core::CODEGEN_TEMPLATE_DIR,
            const std::string& ns1 = "core",
            const std::string& ns2 = "generated",
            const HessianSparsity& sparsity = HessianSparsity::Ones(),
            bool useReverse = true,
            bool ignoreZero = true)
    {
        internal::SparsityPattern pattern;
        pattern.initPattern(sparsity);

        size_t hesDimension = IN_DIM * IN_DIM;

        std::string codeHes =
                internal::CGHelpers::generateHessianSource(
                        this->fCgCppad_,
                        pattern,
                        hesDimension,
                        tmpVarCount_,
                        ignoreZero
                );

        writeCodeFile(templateDir, "/Hessian.tpl.h", "/Hessian.tpl.cpp", outputDir, derivativeName, ns1, ns2, codeHes, "AUTOGENERATED_CODE_PLACEHOLDER");
    }



private:
    //! write code to file
    /*!
     * @param templateDir directory containing template file
     * @param outputDir output directory to write file to
     * @param jacName name of the Jacobian class
     * @param ns1 first layer namespace of the Jacobian class
     * @param ns2 second layer namespace of the Jacobian class
     * @param codeJac source code for computing the Jacobian
     * @param codePlaceholder placeholder pattern in template file to replace with code
     */
    void writeCodeFile(
                const std::string& templateDir,
                const std::string& tplHeaderName,
                const std::string& tplSourceName,
                const std::string& outputDir,
                const std::string& derivativeName,
                const std::string& ns1,
                const std::string& ns2,
                const std::string& codeJac,
                const std::string& codePlaceholder)
    {
        std::cout << "Writing code of " + derivativeName + " to file..."<<std::endl;

        std::string header = internal::CGHelpers::parseFile(templateDir + tplHeaderName);
        std::string source = internal::CGHelpers::parseFile(templateDir + tplSourceName);

        replaceSizesAndNames(header, derivativeName, ns1, ns2);
        replaceSizesAndNames(source, derivativeName, ns1, ns2);

        internal::CGHelpers::replaceOnce(header, "MAX_COUNT", std::to_string(tmpVarCount_));
        internal::CGHelpers::replaceOnce(source, codePlaceholder, codeJac);


        internal::CGHelpers::writeFile(outputDir+"/"+derivativeName+".h", header);
        internal::CGHelpers::writeFile(outputDir+"/"+derivativeName+".cpp", source);


        std::cout << "... Done! Successfully generated " + derivativeName << std::endl;
    }


    //! replaces the size and namespaces in the template file
    /*!
     *
     * @param file filename to pen
     * @param systemName name of the Jacobian
     * @param ns1 first layer namespace
     * @param ns2 second layer namespace
     */
    void replaceSizesAndNames(std::string& file, const std::string& systemName, const std::string& ns1, const std::string& ns2)
    {
        internal::CGHelpers::replaceAll(file, "DERIVATIVE_NAME", systemName);
        internal::CGHelpers::replaceAll(file, "NS1", ns1);
        internal::CGHelpers::replaceAll(file, "NS2", ns2);
        internal::CGHelpers::replaceAll(file, "IN_DIM", std::to_string(IN_DIM));
        internal::CGHelpers::replaceAll(file, "OUT_DIM", std::to_string(OUT_DIM));
    }

    DerivativesCppadSettings settings_;
    size_t tmpVarCount_; //! number of temporary variables in the source code
};

} /* namespace core */
} /* namespace ct */

#endif /* INCLUDE_CT_CORE_FUNCTION_DERIVATIVESCPPAD_CG_H_ */
