// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-      
#include "CartesianOrientationSquaredError.h"
#include <yarp/math/Math.h>
#include <yarp/os/Network.h>
#include <math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;

void CB::CartesianOrientationSquaredError::startPotentialFunction() {
    if(!connectedToInputs) {
        if(!connectToInputs()) {
            cout << "Couldn't connect to YARP input port in startPotentialFunction()..." << endl;
            return;
        }
    }
    start();     // mandatory start function
}

void CB::CartesianOrientationSquaredError::stopPotentialFunction() {
    stop();     // mandatory stop function
}

bool CB::CartesianOrientationSquaredError::updatePotentialFunction() {
    
    Bottle *b[2];
    double offset;
    bool ok = true;
    Vector diff(3);
 
    // get data from ports (should be more safety checks...)
    if(inputPorts.size() != 2) {
        cout << "CartesianOrientationSquaredError::update() -- wrong number of input ports!!" << endl;
        return false;
    }
    b[0] = inputPorts[0]->read(true);
    b[1] = inputPorts[1]->read(true);

    bool b0 = (b[0]==NULL);
    bool b1 = (b[1]==NULL);

    if( b0 || b1 ) {
        if(b0) cout << "XPF - cur invalid" << endl;
        if(b1) cout << "XPF - ref invalid" << endl;
        return ok;
    } else {
       cout << "PF - valid" << endl;
    }
    
    offset = 1;
    if(inputs[0]->size() != 3) inputs[0]->resize(3);
    if(inputs[1]->size() != 3) inputs[1]->resize(3);

    for(int i=0; i<size; i++) {
        (*inputs[0])[i] = b[0]->get(i+offset).asDouble();
        (*inputs[1])[i] = b[1]->get(i+offset).asDouble();
        diff[i] = (*inputs[1])[i] - (*inputs[0])[i];
       
        /*
        if(diff[i] > 0) diff[i] + M_PI;
        if(diff[i] < 0) diff[i] + 2*M_PI;
        
        while(diff[i] > M_PI) {
            diff[i] = diff[i] - M_PI;
        }
        while(diff[i] < M_PI) {
            diff[i] = diff[i] + M_PI;
        }       
        */
    }

    // compute potential and gradient
    gradient = -1.0*diff;
    potential = 0.5*dot(diff,diff);
    
    cout << "ref  -  cur  =  diff (rotation)" << endl;
    for(int i=0; i<size; i++) {
        cout << (*inputs[1])[i] << "    " << (*inputs[0])[i] << "   " << diff[i] << endl;
    }
    cout << endl;


    //cout << "CartesianOrientationSquaredError Potential = " << potential << endl;
    return ok;

}

bool CB::CartesianOrientationSquaredError::connectToInputs() {
    
    bool ok = true;

    if(inputNames.size() != 2) {
        cout << "CartesianOrientationSquaredError::connectToInputs() -- size mismatch!!" << endl;
        return false;
    }

    cout << "CartesianSquaredError::connectToInputs():\n\t" <<
        inputNames[0].c_str() << "\n\t" << inputNames[1].c_str() << endl << endl;

    string posCurName = inputNames[0] + "/data:o";
    string posRefName = inputNames[1] + "/data:o";

    string prefixStr = "/cb/" + getSpace();
    int s = prefixStr.size();
    string tmp0 = inputNames[0];
    string tmp1 = inputNames[1];
    tmp0.erase(0,s);
    tmp1.erase(0,s);

    string posCurNameIn = "/cb/cartesianorientation/squared_error_pf" + tmp0 + "/data:i";
    string posRefNameIn = "/cb/cartesianorientation/squared_error_pf" + tmp1 + "/data:i";

    cout << "CartesianOrientationSquaredError::connectToInputs() -- opening current input port..." << endl;
    ok &= inputPorts[0]->open(posCurNameIn.c_str());
    if(!ok) {
        cout << "CartesianSquaredError::connectToInputs() -- failed opening current input port..." << endl;
        return ok;
    }

    cout << "CartesianOrientationSquaredError::connectToInputs() -- opening reference input port..." << endl;
    ok &= inputPorts[1]->open(posRefNameIn.c_str());
    if(!ok) {
        cout << "ConfigurationSquaredError::connectToInputs() -- failed opening reference input port..." << endl;
        return ok;
    }

    //    Time::delay(0.1);
    cout << "CartesianSquaredError::connectToInputs() -- connecting:\n\t" << 
        posCurName.c_str() << " -> " << posCurNameIn.c_str() << "\n\t" << 
        posRefName.c_str() << " -> " << posRefNameIn.c_str() << endl << endl;

    ok &= Network::connect(posCurName.c_str(),posCurNameIn.c_str(),"tcp");
    if(!ok) {
        cout << "CartesianSquaredError::connectToInputs() -- failed connecting to current input pors...\n\n\n" << endl;
        return ok;
    }
    ok &= Network::connect(posRefName.c_str(),posRefNameIn.c_str(),"tcp");
    if(!ok) {
        cout << "CartesianSquaredError::connectToInputs() -- failed connecting to reference input pors...\n\n\n" << endl;
        return ok;
    }
    
    cout << "CartesianOrientationSquaredError done connecting to YARP input ports..." << endl;

    connectedToInputs = true;
    return ok; 
}


