#ifndef _LSMODEL_
#define _LSMODEL_

#include "Model.h"

class LSModel : public Model {
 private:
    int n_parameters;
    double *model;

    void Initialize(const std::string &input_line) {
	// Expect a single element describing the number of parameters
	// of the model.
	std::stringstream input(input_line);
	input >> n_parameters;

	// Initialize the model.
	model = new double[n_parameters];
	memset(model, 0, sizeof(double) * n_parameters);
    }

 public:
    LSModel(const std::string &input_line) {
	Initialize(input_line);
    }

    ~LSModel() {
	delete [] model;
    }

    void SetUp(const std::vector<Datapoint *> &datapoints) override {
	// Do nothing.
    }

    double ComputeLoss(const std::vector<Datapoint *> &datapoints) override {
	double loss = 0;
#pragma omp parallel for num_threads(FLAGS_n_threads) reduction(+:loss)
	for (int index = 0; index < datapoints.size(); index++) {
	    LSDatapoint *datapoint = (LSDatapoint *)datapoints[index];
	    const std::vector<double> &weights = datapoint->GetWeights();
	    const std::vector<int> &coordinates = datapoint->GetCoordinates();
	    double label = datapoint->GetLabel();
	    double prediction = 0;
	    for (int coordinate_index = 0; coordinate_index < coordinates.size(); coordinate_index++) {
		prediction += weights[coordinate_index] * model[coordinates[coordinate_index]];
	    }
	    loss += (prediction - label) * (prediction - label);
	}
	return loss;
    }

    void ComputeGradient(Datapoint * datapoint, Gradient *gradient) override {
	LSGradient *ls_gradient = (LSGradient *)gradient;
	const std::vector<double> &weights = datapoint->GetWeights();
	const std::vector<int> &coordinates = datapoint->GetCoordinates();
	double label = ((LSDatapoint *)datapoint)->GetLabel();
	double gradient_coefficient = 0;
	ls_gradient->datapoint = datapoint;
	for (int i = 0; i < coordinates.size(); i++) {
	    gradient_coefficient += weights[i] * model[coordinates[i]];
	}
	ls_gradient->gradient_coefficient = 2 * (gradient_coefficient - label);
    }

    void ApplyGradient(Gradient *gradient) override {
	LSGradient *ls_gradient = (LSGradient *)gradient;
	Datapoint *datapoint = ls_gradient->datapoint;
	const std::vector<double> &weights = datapoint->GetWeights();
	const std::vector<int> &coordinates = datapoint->GetCoordinates();
	for (int i = 0; i < coordinates.size(); i++) {
	    double complete_gradient = ls_gradient->gradient_coefficient * weights[i];
	    model[coordinates[i]] -= FLAGS_learning_rate * complete_gradient;
	}
    }

    int NumParameters() {
	return n_parameters;
    }
};

#endif
