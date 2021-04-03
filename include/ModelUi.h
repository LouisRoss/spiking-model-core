#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <map>
#include <chrono>
#include <numeric>

#include "KeyListener.h"
#include "ICommandControlAcceptor.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::unique_ptr;
    using std::map;
    using std::chrono::microseconds;
    using std::ceil;
    using std::floor;

    template<class MODELRUNNERTYPE, class MODELHELPERTYPE>
    class ModelUi
    {
        bool displayOn_ { true };
        bool displayMonitoredNeurons_ { false };
        unsigned int width_ { 100 };
        unsigned int height_ { 100 };
        unsigned int centerWidth_ {};
        unsigned int centerHeight_ {};

        unsigned int windowWidth_ = 15;
        unsigned int windowHeight_ = 15;
        string cls {"\033[2J\033[H"};

        map<string, tuple<int, int>> namedNeurons_ { };

        unique_ptr<ICommandControlAcceptor> commandControl_;

    protected:
        MODELRUNNERTYPE& modelRunner_;
        MODELHELPERTYPE& helper_;

    public:
        ModelUi(MODELRUNNERTYPE& modelRunner, MODELHELPERTYPE& helper, unique_ptr<ICommandControlAcceptor> commandControl) :
            modelRunner_(modelRunner),
            helper_(helper),
            commandControl_(std::move(commandControl))
        {
            width_ = helper_.Width();
            height_ = helper_.Height();

            if (windowWidth_ > width_) windowWidth_ = width_;
            if (windowHeight_ > height_) windowHeight_ = height_;

            centerHeight_ = ceil(windowHeight_ / 2);
            centerWidth_ = ceil(width_ / 2);

            LoadOptionalNamedNeurons();

            commandControl_->Initialize();
        }

        char PrintAndListenForQuit()
        {
            constexpr char KEY_UP = 'A';
            constexpr char KEY_DOWN = 'B';
            constexpr char KEY_LEFT = 'D';
            constexpr char KEY_RIGHT = 'C';

            char c {' '};
            {
                KeyListener listener;

                bool quit {false};
                while (!quit)
                {
                    if (displayOn_)
                    {
                        if (displayMonitoredNeurons_)
                            PrintMonitoredNeurons();
                        else
                            PrintNetworkScan();
                    }

                    commandControl_->AcceptAndExecute();
                    
                    auto gotChar = listener.Listen(50'000, c);
                    if (gotChar)
                    {
                        switch (c)
                        {
                            case KEY_UP:
                                if (centerHeight_ > ceil(windowHeight_ / 2)) centerHeight_--;
                                break;

                            case KEY_DOWN:
                                centerHeight_++;
                                if (centerHeight_ >= floor(height_ - (windowHeight_ / 2))) centerHeight_ = floor(height_ - (windowHeight_ / 2));
                                break;

                            case KEY_LEFT:
                                if (centerWidth_ > ceil(windowWidth_ / 2)) centerWidth_--;
                                break;

                            case KEY_RIGHT:
                                centerWidth_++;
                                if (centerWidth_ >= floor(width_ - (windowWidth_ / 2))) centerWidth_ = floor(width_ - (windowWidth_ / 2));
                                break;

                            case '=':
                            case '+':
                            {
                                auto newPeriod = modelRunner_.EnginePeriod() / 10;
                                if (newPeriod < microseconds(100)) newPeriod = microseconds(100);
                                modelRunner_.EnginePeriod() = newPeriod;
                                break;
                            }

                            case '-':
                            {
                                auto newPeriod = modelRunner_.EnginePeriod() * 10;
                                if (newPeriod > microseconds(10'000'000)) newPeriod = microseconds(10'000'000);
                                modelRunner_.EnginePeriod() = newPeriod;
                                break;
                            }

                            case 'q':
                            case 'Q':
                                quit = true;
                                break;

                            default:
                                break;
                        }
                    }
                }
            }

            cout << "Received keystroke " << c << ", quitting\n";
            return c;
        }

        void PrintNetworkScan()
        {
            cout << cls;

            auto neuronIndex = ((width_ * (centerHeight_ - (windowHeight_ / 2))) + centerWidth_ - (windowWidth_ / 2));
            for (auto high = windowHeight_; high; --high)
            {
                for (auto wide = windowWidth_; wide; --wide)
                {
                    cout << EmitToken(neuronIndex);
                    neuronIndex++;
                }
                cout << '\n';

                neuronIndex += width_ - windowWidth_;
                if (neuronIndex > helper_.Carrier().ModelSize()) neuronIndex = 0;
            }

            cout
                <<  Legend() << ":(" << centerWidth_ << "," << centerHeight_ << ") "
                << " Tick: " << modelRunner_.EnginePeriod().count() << " us "
                << "Iterations: " << modelRunner_.GetModelEngine().GetIterations() 
                << "  Total work: " << modelRunner_.GetModelEngine().GetTotalWork() 
                << "                 \n";

            cout << "Arrow keys to navigate       + and - keys control speed            q to quit\n";
        }

        void PrintMonitoredNeurons()
        {
            cout << cls;

            for (auto& [neuronName, posTuple] : namedNeurons_)
            {
                auto& [ypos, xpos] = posTuple;
                auto neuronIndex = helper_.GetIndex(ypos, xpos);
                auto neuronActivation = helper_.GetNeuronActivation(neuronIndex);
                auto neuronTicksSinceLastSpike = helper_.GetNeuronTicksSinceLastSpike(neuronIndex);
                cout << "Neuron " << std::setw(15) << neuronName << " [" << ypos << ", " << xpos << "] = " << std::setw(4) << neuronIndex << ": ";
                cout << std::setw(5) << neuronActivation << "(" << std::setw(3) << neuronTicksSinceLastSpike << ")";
                cout << std::endl;

                for (auto synapseId = 0; synapseId < SynapticConnectionsPerNode; synapseId++)
                {
                    if (helper_.IsSynapseUsed(neuronIndex, synapseId))
                    {
                        auto presynapticNeuronIndex = helper_.GetPresynapticNeuron(neuronIndex, synapseId);
                        cout << std::setw(20) << presynapticNeuronIndex
                        << "(" << std::setw(3) << helper_.GetSynapticStrength(neuronIndex, synapseId) << ")  ";
                    }
                }

                cout << std::endl;
                cout << std::endl;
            }
            cout << std::endl;

            cout
                <<  Legend() << ": "
                << " Tick: " << modelRunner_.EnginePeriod().count() << " us "
                << "Iterations: " << modelRunner_.GetModelEngine().GetIterations() 
                << "  Total work: " << modelRunner_.GetModelEngine().GetTotalWork() 
                << "                 \n";

            cout << "+ and - keys control speed            q to quit\n";
        }

        void LoadOptionalNamedNeurons()
        {
            const json& configuration = modelRunner_.Configuration();
            auto& modelSection = configuration["Model"];
            if (!modelSection.is_null() && modelSection.contains("Neurons"))
            {
                auto& namedNeuronsElement = modelSection["Neurons"];
                if (namedNeuronsElement.is_object())
                {
                    for (auto& neuron: namedNeuronsElement.items())
                    {
                        auto neuronName = neuron.key();
                        auto positionArray = neuron.value().get<std::vector<int>>();
                        auto xpos = positionArray[0];
                        auto ypos = positionArray[1];

                        namedNeurons_[neuronName] = make_tuple(xpos, ypos);
                    }
                }
            }
        }

        void ParseArguments(int argc, char* argv[])
        {
            for (auto i = 0; i < argc; i++)
            {
                string arg = argv[i];
                if (arg == "-d" || arg == "--nodisplay")
                {
                    displayOn_ = false;
                    cout << "Found " << arg << " flag, turning display off \n";
                }
                if (arg == "-m" || arg == "--monitored")
                {
                    displayMonitoredNeurons_ = true;
                    cout << "Found " << arg << " flag, displaying monitored neurons \n";
                }
            }
        }

        virtual const string& Legend() = 0;
        virtual char EmitToken(unsigned long neuronIndex) = 0;
    };
}
