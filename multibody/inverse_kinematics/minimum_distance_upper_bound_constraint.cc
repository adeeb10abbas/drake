#include "drake/multibody/inverse_kinematics/minimum_distance_upper_bound_constraint.h"

#include <limits>
#include <vector>

#include <Eigen/Dense>

#include "drake/multibody/inverse_kinematics/distance_constraint_utilities.h"
#include "drake/multibody/inverse_kinematics/kinematic_evaluator_utilities.h"
namespace drake {
namespace multibody {
using internal::RefFromPtrOrThrow;

template <typename T>
void MinimumDistanceUpperBoundConstraint::Initialize(
    const MultibodyPlant<T>& plant, systems::Context<T>* plant_context,
    double minimum_distance_upper, double influence_distance_offset,
    const solvers::MinimumValuePenaltyFunction& penalty_function) {
  CheckPlantIsConnectedToSceneGraph(plant, *plant_context);
  CheckBounds(minimum_distance_upper,
              minimum_distance_upper + influence_distance_offset);
  const auto& query_port = plant.get_geometry_query_input_port();
  // Maximum number of SignedDistancePairs returned by calls to
  // ComputeSignedDistancePairwiseClosestPoints().
  const int num_collision_candidates =
      query_port.template Eval<geometry::QueryObject<T>>(*plant_context)
          .inspector()
          .GetCollisionCandidates()
          .size();
  minimum_value_constraint_ =
      std::make_unique<solvers::MinimumValueUpperBoundConstraint>(
          this->num_vars(), minimum_distance_upper, influence_distance_offset,
          num_collision_candidates,
          [&plant, plant_context](const auto& x,
                                  double influence_distance_val) {
            return internal::Distances<T, AutoDiffXd>(plant, plant_context, x,
                                                      influence_distance_val);
          },
          [&plant, plant_context](const auto& x,
                                  double influence_distance_val) {
            return internal::Distances<T, double>(plant, plant_context, x,
                                                  influence_distance_val);
          });
  this->set_bounds(minimum_value_constraint_->lower_bound(),
                   minimum_value_constraint_->upper_bound());
  if (penalty_function) {
    minimum_value_constraint_->set_penalty_function(penalty_function);
  }
}

void MinimumDistanceUpperBoundConstraint::Initialize(
    const planning::CollisionChecker& collision_checker,
    planning::CollisionCheckerContext* collision_checker_context,
    double minimum_distance_upper, double influence_distance_offset,
    const solvers::MinimumValuePenaltyFunction& penalty_function) {
  CheckBounds(minimum_distance_upper,
              minimum_distance_upper + influence_distance_offset);
  minimum_value_constraint_ =
      std::make_unique<solvers::MinimumValueUpperBoundConstraint>(
          collision_checker.plant().num_positions(), minimum_distance_upper,
          influence_distance_offset,
          collision_checker.MaxContextNumDistances(*collision_checker_context),
          [this](const Eigen::Ref<const AutoDiffVecXd>& x,
                 double influence_distance_val) {
            return internal::Distances(*(this->collision_checker_),
                                       this->collision_checker_context_, x,
                                       influence_distance_val);
          },
          [this](const Eigen::Ref<const Eigen::VectorXd>& x,
                 double influence_distance_val) {
            return internal::Distances(*(this->collision_checker_),
                                       this->collision_checker_context_, x,
                                       influence_distance_val);
          });
  this->set_bounds(minimum_value_constraint_->lower_bound(),
                   minimum_value_constraint_->upper_bound());
  if (penalty_function) {
    minimum_value_constraint_->set_penalty_function(penalty_function);
  }
}

MinimumDistanceUpperBoundConstraint::MinimumDistanceUpperBoundConstraint(
    const multibody::MultibodyPlant<double>* const plant,
    double minimum_distance_upper, systems::Context<double>* plant_context,
    solvers::MinimumValuePenaltyFunction penalty_function,
    double influence_distance_offset)
    : solvers::Constraint(1, RefFromPtrOrThrow(plant).num_positions(),
                          Vector1d(0), Vector1d(0)),
      /* The lower and upper bounds will be set to correct value later in
         Initialize() function */
      plant_double_{plant},
      plant_context_double_{plant_context},
      plant_autodiff_{nullptr},
      plant_context_autodiff_{nullptr},
      collision_checker_{nullptr} {
  Initialize(*plant_double_, plant_context_double_, minimum_distance_upper,
             influence_distance_offset, penalty_function);
}

MinimumDistanceUpperBoundConstraint::MinimumDistanceUpperBoundConstraint(
    const multibody::MultibodyPlant<AutoDiffXd>* const plant,
    double minimum_distance_lower, systems::Context<AutoDiffXd>* plant_context,
    solvers::MinimumValuePenaltyFunction penalty_function,
    double influence_distance_offset)
    : solvers::Constraint(1, RefFromPtrOrThrow(plant).num_positions(),
                          Vector1d(0), Vector1d(0)),
      plant_double_{nullptr},
      plant_context_double_{nullptr},
      plant_autodiff_{plant},
      plant_context_autodiff_{plant_context},
      collision_checker_{nullptr} {
  Initialize(*plant_autodiff_, plant_context_autodiff_, minimum_distance_lower,
             influence_distance_offset, penalty_function);
}

MinimumDistanceUpperBoundConstraint::MinimumDistanceUpperBoundConstraint(
    const planning::CollisionChecker* collision_checker,
    double minimum_distance_upper,
    planning::CollisionCheckerContext* collision_checker_context,
    solvers::MinimumValuePenaltyFunction penalty_function,
    double influence_distance_offset)
    : solvers::Constraint(
          1,
          internal::PtrOrThrow(collision_checker,
                               "MinimumDistanceUpperBoundConstraint: "
                               "collision_checker is nullptr")
              ->plant()
              .num_positions(),
          Vector1d(0), Vector1d(0)),
      plant_double_{nullptr},
      plant_context_double_{nullptr},
      plant_autodiff_{nullptr},
      plant_context_autodiff_{nullptr},
      collision_checker_{collision_checker},
      collision_checker_context_{collision_checker_context} {
  Initialize(*collision_checker_, collision_checker_context_,
             minimum_distance_upper, influence_distance_offset,
             penalty_function);
}

void MinimumDistanceUpperBoundConstraint::CheckBounds(
    double minimum_distance_upper, double influence_distance) const {
  if (!std::isfinite(influence_distance)) {
    throw std::invalid_argument(
        "MinimumDistanceUpperBoundConstraint: influence_distance must be "
        "finite.");
  }
  if (influence_distance <= minimum_distance_upper) {
    throw std::invalid_argument(fmt::format(
        "MinimumDistanceUpperBoundConstraint: influence_distance={}, must be "
        "larger than minimum_distance_upper={}; equivalently, "
        "influence_distance_offset={}, but it needs to be positive.",
        influence_distance, minimum_distance_upper,
        influence_distance - minimum_distance_upper));
  }
}

template <typename T>
void MinimumDistanceUpperBoundConstraint::DoEvalGeneric(
    const Eigen::Ref<const VectorX<T>>& x, VectorX<T>* y) const {
  minimum_value_constraint_->Eval(x, y);
}

void MinimumDistanceUpperBoundConstraint::DoEval(
    const Eigen::Ref<const Eigen::VectorXd>& x, Eigen::VectorXd* y) const {
  DoEvalGeneric(x, y);
}

void MinimumDistanceUpperBoundConstraint::DoEval(
    const Eigen::Ref<const AutoDiffVecXd>& x, AutoDiffVecXd* y) const {
  DoEvalGeneric(x, y);
}
}  // namespace multibody
}  // namespace drake
