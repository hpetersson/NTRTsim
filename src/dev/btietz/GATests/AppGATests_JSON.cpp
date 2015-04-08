/*
 * Copyright © 2012, United States Government, as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All rights reserved.
 * 
 * The NASA Tensegrity Robotics Toolkit (NTRT) v1 platform is licensed
 * under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
*/

/**
 * @file AppGATests.cpp
 * @brief An applicaiton to test the efficiency of various genetic algorithms
 * @author Brian Mirletz
 * @copyright Copyright (C) 2014 NASA Ames Research Center
 * $Id$
 */

// This application

// This library

#include "helpers/FileHelpers.h"

#include "neuralNet/Neural Network v2/neuralNetwork.h"

#include <json/json.h>

#include <boost/program_options.hpp>
// The C++ Standard Library
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace po = boost::program_options;

/**
 * The entry point.
 * @param[in] argc the number of command-line arguments
 * @param[in] argv argv[0] is the executable name; argv[1], if supplied, is the
 * suffix for the controller
 * @return 0
 */
int main(int argc, char** argv)
{
    std::cout << "AppGATests" << std::endl;
    
    std::string suffix;
    
    int nSteps;
    
    /* Required for setting up learning file input/output. */
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("steps,s", po::value<int>(&nSteps), "Number of steps per episode to run. Default=60K (60 seconds)")
        ("learning_controller,l", po::value<std::string>(&suffix), "Which learned controller to write to or use. Default = default")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    std::string fileName = "Config.ini";
    std::string path = "bmirletz/GATests/";
    
    std::string fullPath = FileHelpers::getResourcePath(path);
    
        Json::Value root; // will contains the root value after parsing.
    Json::Reader reader;
    
    std::string controlFilename = fullPath + suffix;
    
    bool parsingSuccessful = reader.parse( FileHelpers::getFileString(controlFilename.c_str()), root );
    if ( !parsingSuccessful )
    {
        // report to the user the failure and their locations in the document.
        std::cout << "Failed to parse configuration\n"
            << reader.getFormattedErrorMessages();
        throw std::invalid_argument("Bad filename for JSON");
    }

    Json::Value feedbackParams = root.get("feedbackVals", "UTF-8");
    feedbackParams = feedbackParams.get("params", "UTF-8");
    
    // Setup neural network
    const int numberOfInputs  = feedbackParams.get("numStates", "UTF-8").asInt();
    const int numberOfOutputs = feedbackParams.get("numActions", "UTF-8").asInt();
    
    std::string nnFile = fullPath + feedbackParams.get("neuralFilename", "UTF-8").asString();
    
    neuralNetwork* nn = new neuralNetwork(numberOfInputs,numberOfInputs*2, numberOfOutputs);
    
    nn->loadWeights(nnFile.c_str());
    
    std::vector<double> state(numberOfInputs, 1.0);
    double goal = 1.0 * (double) numberOfOutputs;

    
    int steps = 0;
    double *inputs = new double[numberOfInputs];    
    for (std::size_t i = 0; i < state.size(); i++)
    {
        inputs[i]=state[i] / 2.0 + 0.5;
    }
    
    double *output = nn->feedForwardPattern(inputs);
    double score1 = 0.0;
    for(std::size_t i = 0; i < numberOfOutputs; i++)
    {
        score1 += output[i];
    }
    
    std::vector<double> scores;
    scores.push_back(score1);
    scores.push_back(0.0);
    
    Json::Value prevScores = root.get("scores", Json::nullValue);
    
    Json::Value subScores;
    subScores["distance"] = scores[0];
    subScores["energy"] = 0.0;
    
    prevScores.append(subScores);
    root["scores"] = prevScores;
    
    std::ofstream payloadLog;
    payloadLog.open(controlFilename.c_str(),std::ofstream::out);
    
    payloadLog << root << std::endl;
    

    //Teardown is handled by delete, so that should be automatic
    return 0;
}
