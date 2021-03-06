#include "Swarm.h"

Swarm::Swarm() {
}

void Swarm::setup(int nA, int nD) {
	//Initialise parameters to default values
	//---------------------------------------
	numAgents = nA;
	numDimensions = nD;
	//Used to update swarm sizes when sliders change
	currNumAgents = numAgents;
	currNumDimensions = numDimensions;

	boolElitist = true;
	boolDisturbBeforeUpdate = true;
	boolDisturbSeparately = true;

	initDt = 0.05;
	initDamt = 0.8;
	initDexp = 1.0;
	initUpdateAmt = 1.0;
	initUpdateExp = 1.0;
	initValue = 0.0;
	initGoals = 0.5;
	singleThresh = initDt;

	//Initialise general toggle panel
	initPanelX = 10;
	initPanelY = 10;
	controlTogglesPanel = new ofxDatGui(initPanelX, initPanelY);
	controlTogglesPanel->addHeader("General Controls");
	ofxDatGuiLog::quiet();

	controlTogglesPanel->addSlider("Number of Agents", 2, 100)->bind(numAgents);
	controlTogglesPanel->getSlider("Number of Agents")->setPrecision(0);

	controlTogglesPanel->addSlider("Number of Dimensions", 1, 20)->bind(numDimensions);
	controlTogglesPanel->getSlider("Number of Dimensions")->setPrecision(0);

	controlTogglesPanel->addToggle("Elitist Approach", boolElitist);

	controlTogglesPanel->addSlider("Updates per Loop", 1, 50)->bind(updatesPerLoop);
	controlTogglesPanel->getSlider("updates per loop")->setPrecision(0);

	controlTogglesPanel->addSlider("Loop Every", 1, 100)->bind(loopEvery);
	controlTogglesPanel->getSlider("loop every")->setPrecision(0);

	controlTogglesPanel->addToggle("Disturb Before Update", boolDisturbBeforeUpdate);
	controlTogglesPanel->addToggle("Separate Disturbance Thresholds", boolDisturbSeparately);
	
	controlTogglesPanel->addSlider("Single Disturbance Threshold", 0, 1)->bind(singleThresh);
	controlTogglesPanel->onToggleEvent(this, &Swarm::onToggleEvent);

	//Resize vector to hold the vectors of sliders for each panel/parameter
	for (int i = 0; i < numDimensions; i++) {
		addParamDimension();
	}

	initGuiPanels(numDimensions);

	//Initialise agents with defaults
	//---------------------------------------
	for (int i = 0; i < numAgents; i++) {
		vector<float> tempValues;

		vector<float> tempPrevious = {};
		vector<float> tempNext = {};

		for (int i = 0; i < numDimensions; i++) {
			float t = ofRandom(0, 1);
			tempValues.push_back(t);

			tempPrevious.push_back(0);
			tempNext.push_back(t);
		}
		//Initializing lerp vectors
		previousValues.push_back(tempPrevious);
		nextValues.push_back(tempNext);
		lerpedValues.push_back(tempPrevious);

		Agent a = Agent(tempValues);
		agents.push_back(a);
	}

}

//--------------------------------------------------------------
void Swarm::update() {
	//oscUpdateParameters();
	checkForNumChanges();

	interpPhase = (ofGetFrameNum() % loopEvery) / (float)loopEvery;
	if (interpPhase == 0) {

		for (int i = 0; i < updatesPerLoop; i++) {
			updateSwarm();
		}

		//Passing on the old values and updating the new
		previousValues = nextValues;
		for (int i = 0; i < numAgents; i++) {
			for (int d = 0; d < numDimensions; d++) {
				nextValues[i][d] = agents[i].values[d];
			}
		}
	}
	else {
		lerpValues();
	}
}

//--------------------------------------------------------------
//TODO: set all goals
void Swarm::setGoals(float x, float y) {
	goals[0] = x;
	goals[1] = y;
}
void Swarm::setGoals(const vector<float>& newGoals) {
	for (int i = 0; i < newGoals.size(); i++) {
		goals[i] = newGoals[i];
	}
}

//--------------------------------------------------------------
void Swarm::updateSwarm() {
	if (boolDisturbBeforeUpdate) {
		//Supply either a vector of thresholds or a single threshold for all dimensions
		if (boolDisturbSeparately)
			disturbAgents(distThreshs, distExps, distAmts);
		else disturbAgents(singleThresh, distExps, distAmts);

		updateFitnesses(goals);
		updateAgents(boolElitist, updateAmts, updateExps);
	}
	else {
		updateFitnesses(goals);
		updateAgents(boolElitist, updateAmts, updateExps);

		if (boolDisturbSeparately)
			disturbAgents(distThreshs, distExps, distAmts);
		else disturbAgents(singleThresh, distExps, distAmts);
	}
}

void Swarm::setElitist(bool b) {
	boolElitist = b;
	controlTogglesPanel->getToggle("Elitist Approach")->setChecked(b);
}

void Swarm::setLoopEvery(int e) {
	loopEvery = e;
}
void Swarm::setUpdatesPerLoop(int e) {
	updatesPerLoop = e;
}

//--------------------------------------------------------------
void Swarm::updateAgents(bool elitist, const vector<float>& updateAmt, const vector<float>& updateExp) {
	for (int i = 0; i < agents.size(); i++) {
		if (i != bestAgentIndx) { //don't update best agent
			int bestNeighborIndx = findBestNeighbor(i);
			agents[i].update(agents[bestNeighborIndx], agents[bestAgentIndx], updateAmt, updateExp, elitist);
		}
	}
}

//--------------------------------------------------------------
void Swarm::disturbAgents(const vector<float>& thresholds, const vector<float>& exponents, const vector<float>& amounts) {
	if (thresholds.size() != numDimensions || exponents.size() != numDimensions || amounts.size() != numDimensions) {
		cout << "Swarm::disturbAgents -> vector sizes don't match" << endl;
		return;
	}
	else {
		for (int i = 0; i < agents.size(); i++) {
			if (i != bestAgentIndx) //Don't disturb the best agent
				agents[i].disturb(thresholds, exponents, amounts);
		}
	}
}
//-------------------overloaded-------------------
void Swarm::disturbAgents(float threshold, const vector<float>& exponents, const vector<float>& amounts) {
	if (exponents.size() != numDimensions || amounts.size() != numDimensions) {
		cout << "Swarm::disturbAgents -> vector sizes don't match" << endl;
		return;
	}
	else {
		for (int i = 0; i < agents.size(); i++) {
			if (i != bestAgentIndx) //Don't disturb the best agent
				agents[i].disturb(threshold, exponents, amounts);
		}
	}
}

//--------------------------------------------------------------
int Swarm::findBestNeighbor(int currentAgent) {
	float minDist = FLT_MAX;
	int firstNeighbor = 0;
	int secondNeighbor = 0;

	//Find first neighbor
	for (int i = 0; i < agents.size(); i++) {
		//Skip if we're testing against itself
		if (i == currentAgent)
			continue;

		float d = agents[currentAgent].calcDistance(agents[i]);
		if (d < minDist) {
			firstNeighbor = i;
			minDist = d;
		}
	}
	//Find second neighbor
	minDist = FLT_MAX; // reset minDist
	for (int i = 0; i < agents.size(); i++) {
		//Skip if we're testing against itself or the first neighbor
		if (i == currentAgent || i == firstNeighbor)
			continue;

		float d = agents[currentAgent].calcDistance(agents[i]);
		if (d < minDist) {
			secondNeighbor = i;
			minDist = d;
		}
	}

	if (agents[firstNeighbor].getFitness() < agents[secondNeighbor].getFitness())
		return firstNeighbor;
	else return secondNeighbor;
}

//--------------------------------------------------------------
void Swarm::updateFitnesses(const vector<float>& goals) {
	float minScore = FLT_MAX;
	//int indexOfBest = 0;

	for (int i = 0; i < agents.size(); i++) {
		agents[i].updateFitness(goals);

		float f = agents[i].getFitness();
		if (f < minScore) {
			bestAgentIndx = i; //update index of globally best agent
			minScore = f;
		}
	}
}

//--------------------------------------------------------------
int Swarm::size() {
	return agents.size();
}

void Swarm::setDrawColour(ofColor c) {
	drawColour = c;
}
void Swarm::setBestColour(ofColor c) {
	bestColour = c;
}

//--------------------------------------------------------------
void Swarm::draw() {
	int lineWidth = 3 * ofGetWidth() / 4;
	int initX = 350;
	int padding = 100;
	int height = ofGetHeight() - 2 * padding;

	/* padding       (height-padding) */

	for (int d = 0; d < numDimensions; d++) {
		float y = padding + height * ((float)d / numDimensions);

		ofSetColor(255);
		ofSetLineWidth(6);
		//Draw SS lines
		ofDrawLine(initX, y, initX + lineWidth, y);
		//Draw target values for each dimension
		ofDrawCircle(initX + goals[d] * lineWidth, y, 20);

		//Draw each agent's position in that dimension
		ofSetColor(200, 0, 0, 100);
		for (int i = 0; i < numAgents; i++) {
			if (i != bestAgentIndx) {
				ofDrawCircle(initX + lerpedValues[i][d] * lineWidth, y, 8);
			}
		}

		//Draw best agent in that dimension
		ofSetColor(50, 200, 255, 150);
		ofDrawCircle(initX + lerpedValues[bestAgentIndx][d] * lineWidth, y, 8);
	}
}

void Swarm::lerpValues() {
	for (int i = 0; i < numAgents; i++) {
		for (int d = 0; d < numDimensions; d++) {
			lerpedValues[i][d] = ofLerp(previousValues[i][d], nextValues[i][d], interpPhase);
		}
	}
}

//--------------------------------------------------------------
void Swarm::resizeSwarm(int newSize) {
	int currentSize = agents.size();
	int diff = newSize - currentSize;
	//If we should increase
	if (diff > 0) {
		//Keep adding agents until size is as desired
		while (agents.size() != newSize) {
			//Initialize agents to random position
			vector<float> temp;
			for (int j = 0; j < numDimensions; j++) {
				temp.push_back(ofRandom(0, 1));
			}
			previousValues.push_back(temp);
			nextValues.push_back(temp);
			Agent a = Agent(temp);
			agents.push_back(a);
		}
	}
	//Else we should decrease (only if there is more than one agent in the swarm)
	else if (diff < 0) {
		if (currentSize >= 2) {
			//Keep removing agents until size is as desired
			while (agents.size() != newSize) {
				previousValues.pop_back();
				nextValues.pop_back();
				agents.pop_back();
			}
		}
		else cout << "Not enough agents to remove" << endl;
	}
}

//--------------------------------------------------------------
void Swarm::resizeDimensions(int newNum) {
	int diff = newNum - numDimensions;

	//If we should increase:
	if (diff > 0) {
		//Add dimensions to all agents
		for (int i = 0; i < agents.size(); i++) {
			for (int k = 0; k < diff; k++) {
				agents[i].addDimension(initValue);

				previousValues[i].push_back(initValue);
				nextValues[i].push_back(initValue);
			}
		}

		//Add gui sliders and elements to the parameter vectors to match
		for (int k = 0; k < diff; k++) {
			addParamDimension();
		}
	}
	//If we should decrease:
	else if (diff < 0) {
		//Remove dimensions from all agents (only if there are at least 2 left)
		if (numDimensions + diff >= 2) {
			for (int i = 0; i < agents.size(); i++) {
				for (int k = 0; k < abs(diff); k++) {
					agents[i].removeDimension();
					previousValues[i].pop_back();
					nextValues[i].pop_back();
				}
			}
			//Remove dimension parameters to match
			for (int k = 0; k < abs(diff); k++) {
				removeDimParams();
			}
		}
		else cout << "Swarm::resizeDims -> can't remove dimensions" << endl;
	}
}

//--------------------------------------------------------------
void Swarm::positionGuiPanels() {
	for (int i = 0; i < panels.size(); i++) {
		if (i == 0) {
			panels[i]->setPosition(controlTogglesPanel->getPosition().x, controlTogglesPanel->getPosition().y + controlTogglesPanel->getHeight());
		}
		else {
			int newY = panels[i - 1]->getPosition().y + panels[i - 1]->getHeight();
			panels[i]->setPosition(initPanelX, newY);
		}
	}
}

void Swarm::addParamDimension() {
	distThreshs.push_back(initDt);
	distAmts.push_back(initDamt);
	distExps.push_back(initDexp);
	updateAmts.push_back(initUpdateAmt);
	updateExps.push_back(initUpdateExp);
	goals.push_back(initValue);
}

void Swarm::removeDimParams() {
	distThreshs.pop_back();
	distAmts.pop_back();
	distExps.pop_back();
	updateAmts.pop_back();
	updateExps.pop_back();
	goals.pop_back();
}


void Swarm::showGui(bool b) {
	controlTogglesPanel->setVisible(b);

	for (int i = 0; i < panels.size(); i++) {
		panels[i]->setVisible(b);
	}
}

void Swarm::setSingleThresh(float dt) {
	singleThresh = dt;
}

void Swarm::resetAllTo(const vector<float>& target) {
	if (target.size() != numDimensions)
		cout << "resetAllTo: Sizes don't match" << endl;
	else {
		for (int i = 0; i < numAgents; i++) {
			for (int d = 0; d < numDimensions; d++) {
				agents[i].values[d] = target[d];
				previousValues[i][d] = target[d];
				nextValues[i][d] = target[d];
				lerpedValues[i][d] = target[d];
			}
		}
	}
}

void Swarm::resetAllTo(float f) {
	for (int i = 0; i < numAgents; i++) {
		for (int d = 0; d < numDimensions; d++) {
			agents[i].values[d] = f;
			previousValues[i][d] = f;
			nextValues[i][d] = f;
			lerpedValues[i][d] = f;
		}
	}
}

void Swarm::setDistThreshs(const vector<float>& dt) {
	if (numDimensions == dt.size()) {
		for (int i = 0; i < dt.size(); i++) {
			distThreshs[i] = dt[i];
		}
	}
	else {
		cout << "SetDistThreshs: vectors don't match!" << endl;
	}
}

void Swarm::setDistAmts(const vector<float>& amts) {
	if (numDimensions == amts.size()) {
		for (int i = 0; i < amts.size(); i++) {
			distAmts[i] = amts[i];
		}
	}
	else {
		cout << "SetDistAmts: vectors don't match!" << endl;
	}
}

void Swarm::setDistExps(const vector<float>& exps) {
	if (numDimensions == exps.size()) {
		for (int i = 0; i < exps.size(); i++) {
			distExps[i] = exps[i];
		}
	}
	else {
		cout << "SetDistExps: vectors don't match!" << endl;
	}
}

void Swarm::setUpdateAmts(const vector<float>& amts) {
	if (numDimensions == amts.size()) {
		for (int i = 0; i < amts.size(); i++) {
			updateAmts[i] = amts[i];
		}
	}
	else {
		cout << "SetUpdateAmts: vectors don't match!" << endl;
	}
}

void Swarm::setUpdateExps(const vector<float>& exps) {
	if (numDimensions == exps.size()) {
		for (int i = 0; i < exps.size(); i++) {
			updateExps[i] = exps[i];
		}
	}
	else {
		cout << "SetUpdateExps: vectors don't match!" << endl;
	}
}

vector<vector<float> >& Swarm::getLerpedValues() {
	return lerpedValues;
}

//--------------------------------------------------------------
void Swarm::setDisturbSeparately(bool b) {
	boolDisturbSeparately = b;
	controlTogglesPanel->getToggle("Separate Disturbance Thresholds")->setChecked(b);
}

//--------------------------------------------------------------
void Swarm::onToggleEvent(ofxDatGuiToggleEvent e) {
	if (e.target->is("Elitist Approach")) {
		boolElitist = !boolElitist;
		cout << "Elitist Approach: " << boolElitist << endl;
	}
	else if (e.target->is("Disturb Before Update")) {
		boolDisturbBeforeUpdate = !boolDisturbBeforeUpdate;
		cout << "Disturb Before Update: " << boolDisturbBeforeUpdate << endl;
	}
	else if (e.target->is("Separate Disturbance Thresholds")) {
		boolDisturbSeparately = !boolDisturbSeparately;
		cout << "Separate Disturbance Thresholds: " << boolDisturbSeparately << endl;
	}
}

//--------------------------------------------------------------
void Swarm::initGuiPanels(int num) {
	if (distThreshs.size() != num) {
		cout << "initGuiPanels: sizes don't match" << endl;
		return;
	}

	vector<string> panelNames = {
		"Disturbance Thresholds",
		"Disturbance Amounts" ,
		"Disturbance Amount Exponents",
		"Update Amounts",
		"Update Amount Exponents",
		"Goals"
	};

	for (int i = 0; i < panelNames.size(); i++) {
		panels.push_back(new ofxDatGui());
		panels[i]->addHeader(panelNames[i]);
	}

	for (int i = 0; i < num; i++) {
		stringstream s;
		s << "Dimension " << i + 1;

		//Disturbance Thresholds
		panels[0]->addSlider(s.str(), 0, 1);
		panels[0]->getSlider(s.str())->bind(distThreshs[i]);
		//Disturbance Amounts
		panels[1]->addSlider(s.str(), 0, 2);
		panels[1]->getSlider(s.str())->bind(distAmts[i]);
		//Disturbance Exponents
		panels[2]->addSlider(s.str(), 0, 4);
		panels[2]->getSlider(s.str())->bind(distExps[i]);
		//Update Amounts
		panels[3]->addSlider(s.str(), 0, 2);
		panels[3]->getSlider(s.str())->bind(updateAmts[i]);
		//Update amount Exponents
		panels[4]->addSlider(s.str(), 0, 4);
		panels[4]->getSlider(s.str())->bind(updateExps[i]);
		//Goals
		panels[5]->addSlider(s.str(), 0, 1);
		panels[5]->getSlider(s.str())->bind(goals[i]);
	}

	positionGuiPanels();
}

//--------------------------------------------------------------
void Swarm::deleteGuiPanels() {
	for (int i = 0; i < panels.size(); i++) {
		panels[i]->~ofxDatGui();
		delete panels[i];
	}

	panels.clear();
}

//--------------------------------------------------------------
void Swarm::checkForNumChanges() {
	if (currNumAgents != numAgents) {
		setNumAgents(numAgents);
	}
	if (currNumDimensions != numDimensions) {
		setNumDimensions(numDimensions);
	}
}

//--------------------------------------------------------------
void Swarm::setNumAgents(int num) {
	resizeSwarm(num);
	numAgents = num;
}

//--------------------------------------------------------------
void Swarm::setNumDimensions(int num) {
	deleteGuiPanels();
	resizeDimensions(num);
	initGuiPanels(num);

	numDimensions = num;
	currNumDimensions = num;
	cout << "Dimensions resized!" << endl;
}
