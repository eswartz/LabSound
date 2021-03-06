#include "ExampleBaseApp.h"

// Illustrates 3d sound spatialization and doppler shift
struct SpatializationApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        auto ac = context.get();
        
        SoundBuffer train("samples/trainrolling.wav", context->sampleRate());
		std::shared_ptr<OscillatorNode> osc; 

        auto panner = std::make_shared<PannerNode>(context->sampleRate());
        std::shared_ptr<AudioBufferSourceNode> trainNode;
        {
            ContextGraphLock g(context, "spatialization");
            ContextRenderLock r(context, "spatialization");
			
			//osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
			//osc->connect(ac, panner.get(), 0, 0);
			//osc->start(0);

            panner->connect(ac, context->destination().get(), 0, 0);
            trainNode = train.play(r, panner, 0.0f);
        }
        
        if (trainNode)
        {
            trainNode->setLooping(true);
            context->listener()->setPosition(0, 0, 0);
            panner->setVelocity(5, 0, 0);
            
            const int seconds = 10;
            float halfTime = seconds * 0.5f;
            for (float i = 0; i < seconds; i += 0.01f)
            {
                float x = (i - halfTime) / halfTime;
                // Put position a +up && +front, because if it goes right through the
                // listener at (0, 0, 0) it abruptly switches from left to right.
                panner->setPosition(x, 0.1f, 0.1f);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
        }
        
        LabSound::finish(context);
    }
};
