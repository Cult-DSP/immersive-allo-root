#include "al/app/al_App.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "al/graphics/al_Mesh.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_VAOMesh.hpp"
#include "al/io/al_ControlNav.hpp"
#include "al/io/al_File.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Vec.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include <iostream>
#include <string>

#include "shaderToSphere.hpp"
#include "SpatialRootAlloHost.hpp"

class ImmersiveAlloRootApp : public al::App {
public:
  // Shader paths
  std::vector<std::string> fragOptions = {
    "Sphere1B.frag",
    "CanyonSphere.frag",
    "ScatSphere.frag",
    "smoothBlue.frag",
    "CellNoise.frag"
  };
  std::string shaderFolder = "../miniShader/demoShaders/";
  float STARTING_TIME = 0.0f;
  float PLAYBACK_SPEED = 1.0f;
  bool printTime = false;

  // Spatial Root host
  SpatialRootAlloHost spatialRoot;
  SpatialRootHostConfig srConfig;

  // Graphics
  al::SearchPaths searchPaths;
  ShadedSphere shadedSphere;
  std::string vertPath;
  std::vector<std::string> fragPathOptions;

  // Parameters
  al::Parameter globalTime{"globalTime", "", STARTING_TIME, 0.0, 300.0};
  al::ParameterBool running{"running", "0", false};
  al::ParameterInt currentFragIndex{"currentFragIndex", "0", 0, 0, 10};

  // Spatial Root parameters
  al::Parameter masterGainDb{"masterGainDb", "", 0.0f, -60.0f, 12.0f};
  al::Parameter dbapFocus{"dbapFocus", "", 1.5f, 0.1f, 5.0f};
  al::Parameter speakerMixDb{"speakerMixDb", "", 0.0f, -60.0f, 12.0f};
  al::Parameter subMixDb{"subMixDb", "", 0.0f, -60.0f, 12.0f};
  al::ParameterInt elevationMode{"elevationMode", "", 0, 0, 2};
  al::ParameterBool showOverlay{"showOverlay", "", true};

  bool showHelp = true;
  int currentFlag = -1;
  bool srInitialized = false;

  void onInit() override {
    // Graphics initialization
    searchPaths.addSearchPath(al::File::currentPath() + shaderFolder);

    al::FilePath vertPathSource = searchPaths.find("standard.vert");
    if (vertPathSource.valid()) {
      vertPath = vertPathSource.filepath();
      std::cout << "Found vertex shader: " << vertPath << std::endl;
    } else {
      std::cout << "Could not find vertex shader" << std::endl;
    }

    fragPathOptions.clear();
    for (const auto &fragOption : fragOptions) {
      al::FilePath fp = searchPaths.find(fragOption);
      if (fp.valid()) {
        fragPathOptions.push_back(fp.filepath());
        std::cout << "Found fragment shader: " << fp.filepath() << std::endl;
      } else {
        std::cout << "Fragment shader not found: " << fragOption << std::endl;
      }
    }

    // Configure Spatial Root
    srConfig.sampleRate = 48000;
    srConfig.blockSize = 512;
    srConfig.scenePath = "assets/scenes/example.scene.lusid.json";
    srConfig.layoutPath = "assets/layouts/allosphere.layout.json";
  }

  void onCreate() override {
    shadedSphere.setSphere(15.0, 20);
    if (!vertPath.empty() && currentFragIndex < (int)fragPathOptions.size() && !fragPathOptions[currentFragIndex].empty()) {
      shadedSphere.setShaders(vertPath, fragPathOptions[currentFragIndex]);
    }
    shadedSphere.update();

    // Initialize Spatial Root (non-blocking on error)
    if (!spatialRoot.setup(srConfig)) {
      std::cerr << "Spatial Root setup failed: " << spatialRoot.lastError() << std::endl;
    } else {
      srInitialized = true;
      spatialRoot.setPaused(true);
      std::cout << "Spatial Root initialized successfully." << std::endl;
    }
  }

  void onAnimate(double dt) override {
    if (running) {
      globalTime = globalTime + (dt * PLAYBACK_SPEED);
      if (printTime) {
        std::cout << globalTime << std::endl;
      }
    }

    if (currentFragIndex.get() != currentFlag) {
      if (currentFragIndex < (int)fragPathOptions.size() && !fragPathOptions[currentFragIndex].empty()) {
        shadedSphere.setShaders(vertPath, fragPathOptions[currentFragIndex]);
        shadedSphere.update();
      }
      currentFlag = currentFragIndex.get();
    }

    // Push Spatial Root parameter changes
    if (srInitialized) {
      spatialRoot.setPaused(!running);
      spatialRoot.setMasterGainDb(masterGainDb);
      spatialRoot.setDbapFocus(dbapFocus);
      spatialRoot.setSpeakerMixDb(speakerMixDb);
      spatialRoot.setSubMixDb(subMixDb);
      spatialRoot.setElevationMode(elevationMode);
      spatialRoot.update(dt);
    }
  }

  void onDraw(al::Graphics &g) override {
    g.lens().eyeSep(0.0);
    g.clear(0.0);
    g.shader(shadedSphere.shader());
    shadedSphere.setUniformFloat("u_time", globalTime);
    shadedSphere.update();
    shadedSphere.draw(g);
  }

  bool onKeyDown(const al::Keyboard &k) override {
    switch (k.key()) {
    case ' ':
      running = !running;
      std::cout << (running ? "▶ Started" : "⏸ Paused") << std::endl;
      break;

    case 'h':
    case 'H':
      showOverlay = !showOverlay;
      break;

    case 'e':
    case 'E':
      elevationMode = (elevationMode + 1) % 3;
      std::cout << "Elevation mode: " << elevationMode << std::endl;
      break;

    case 'g':
      masterGainDb = masterGainDb - 1.0f;
      std::cout << "Master gain: " << masterGainDb << " dB" << std::endl;
      break;
    case 'G':
      masterGainDb = masterGainDb + 1.0f;
      std::cout << "Master gain: " << masterGainDb << " dB" << std::endl;
      break;

    case 'f':
      dbapFocus = dbapFocus * 0.8f;
      std::cout << "DBAP focus: " << dbapFocus << std::endl;
      break;
    case 'F':
      dbapFocus = dbapFocus * 1.25f;
      std::cout << "DBAP focus: " << dbapFocus << std::endl;
      break;

    case 's':
      subMixDb = subMixDb - 1.0f;
      std::cout << "Sub mix: " << subMixDb << " dB" << std::endl;
      break;
    case 'S':
      subMixDb = subMixDb + 1.0f;
      std::cout << "Sub mix: " << subMixDb << " dB" << std::endl;
      break;

    case 'm':
      speakerMixDb = speakerMixDb - 1.0f;
      std::cout << "Speaker mix: " << speakerMixDb << " dB" << std::endl;
      break;
    case 'M':
      speakerMixDb = speakerMixDb + 1.0f;
      std::cout << "Speaker mix: " << speakerMixDb << " dB" << std::endl;
      break;

    default:
      if (k.key() >= '1' && k.key() <= '9') {
        running = false;
        globalTime = 0.0f;
        int idx = static_cast<int>(k.key() - '1');
        if (idx < static_cast<int>(fragPathOptions.size())) {
          if (currentFragIndex == idx) {
            std::cout << "Shader [" << (idx + 1) << "] already active" << std::endl;
            return true;
          }
          currentFragIndex = idx;
          std::cout << "Switched to shader [" << (idx + 1) << "] " << fragPathOptions[currentFragIndex] << std::endl;
        } else {
          std::cerr << "No shader for key '" << k.key() << "'" << std::endl;
        }
      }
      break;
    }
    return true;
  }

  void onSound(al::AudioIOData& io) override {
    spatialRoot.renderAudio(io);
  }
};

int main() {
  ImmersiveAlloRootApp app;
  app.title("Immersive Allo Root");
  app.configureAudio(48000, 512, 60, 0);
  app.start();
  return 0;
}
