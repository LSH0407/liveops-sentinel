#include "ObsClient.h"

namespace obs {
bool ObsClient::connect(const std::string&, int, const std::string&) { return false; }
bool ObsClient::isConnected() const { return false; }
bool ObsClient::startStream(){return false;} 
bool ObsClient::stopStream(){return false;}
bool ObsClient::startRecord(){return false;} 
bool ObsClient::stopRecord(){return false;}
bool ObsClient::setCurrentProgramScene(const std::string&){return false;}
bool ObsClient::getSceneList(std::vector<std::string>&){return false;}
std::optional<Stats> ObsClient::getStats(){ return Stats{}; }
std::optional<VideoSettings> ObsClient::getVideoSettings(){ return VideoSettings{}; }
}
