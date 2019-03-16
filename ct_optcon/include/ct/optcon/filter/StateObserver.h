/**********************************************************************************************************************
This file is part of the Control Toolbox (https://adrlab.bitbucket.io/ct), copyright by ETH Zurich, Google Inc.
Licensed under Apache2 license (see LICENSE file in main directory)
**********************************************************************************************************************/

#pragma once

#include "FilterBase.h"
#include "FilterSettings.h"
#include "EstimatorBase.h"
#include "LTIMeasurementModel.h"

namespace ct {
namespace optcon {

/*!
 * \ingroup Filter
 *
 * \brief State Observer estimates the state by combining the state estimator as well as the system and measurement
 *        models.
 */
template <size_t OUTPUT_DIM, size_t STATE_DIM, size_t CONTROL_DIM, class ESTIMATOR, typename SCALAR = double>
class StateObserver : public FilterBase<OUTPUT_DIM, STATE_DIM, CONTROL_DIM, SCALAR>
{
public:
    static_assert(STATE_DIM == ESTIMATOR::STATE_D, "Observer and estimator dimensions have to be the same!");

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using Base = FilterBase<OUTPUT_DIM, STATE_DIM, CONTROL_DIM, SCALAR>;
    using typename Base::control_vector_t;
    using typename Base::output_vector_t;
    using typename Base::state_vector_t;
    using typename Base::Time_t;
    using state_matrix_t = ct::core::StateMatrix<STATE_DIM, SCALAR>;
    using output_matrix_t = ct::core::OutputMatrix<OUTPUT_DIM, SCALAR>;
    using output_state_matrix_t = ct::core::OutputStateMatrix<OUTPUT_DIM, STATE_DIM, SCALAR>;

    using SensitivityApproximation_t =
        ct::core::SensitivityApproximation<STATE_DIM, CONTROL_DIM, STATE_DIM / 2, STATE_DIM / 2, SCALAR>;

    //! Constructor.
    StateObserver(std::shared_ptr<ct::core::ControlledSystem<STATE_DIM, CONTROL_DIM, SCALAR>> system,
        const SensitivityApproximation_t& sensApprox,
        const output_state_matrix_t& C,
        const output_matrix_t& D,
        const ESTIMATOR& estimator,
        const state_matrix_t& Q,
        const output_matrix_t& R,
        const state_matrix_t& dFdv);

    //! Constructor using the observer settings.
    StateObserver(std::shared_ptr<ct::core::ControlledSystem<STATE_DIM, CONTROL_DIM, SCALAR>> system,
        const SensitivityApproximation_t& sensApprox,
        const ESTIMATOR& estimator,
        const StateObserverSettings<OUTPUT_DIM, STATE_DIM, SCALAR>& so_settings);

    //! Virtual destructor.
    virtual ~StateObserver() = default;

    //! Implementation of the filter method.
    state_vector_t filter(const control_vector_t& u,
        const output_vector_t& y,
        const ct::core::Time& dt,
        const Time_t& t) override;

    //! Observer predict method.
    virtual state_vector_t predict(const control_vector_t& u, const ct::core::Time& dt, const Time_t& t);

    //! Observer update method.
    virtual state_vector_t update(const output_vector_t& y, const ct::core::Time& dt, const Time_t& t);

protected:
    ESTIMATOR estimator_;                                   //! Estimator used to filter the state.
    CTSystemModel<STATE_DIM, CONTROL_DIM, SCALAR> f_;       //! System model for propagating the system.
    LTIMeasurementModel<OUTPUT_DIM, STATE_DIM, SCALAR> h_;  //! Observation model used to calculate the output error.
    state_matrix_t Q_;                                      //! Filter Q matrix.
    output_matrix_t R_;                                     //! Filter R matrix.
};

}  // namespace optcon
}  // namespace ct
