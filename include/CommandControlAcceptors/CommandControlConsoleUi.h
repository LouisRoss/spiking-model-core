#pragma once

#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <chrono>

#include <nlohmann/json.hpp>

#include "IModelRunner.h"
#include "ICommandControlAcceptor.h"
#include "IModelHelper.h"
#include "KeyListener.h"

namespace embeddedpenguins::core::neuron::model
{
    using std::cout;
    using std::string;
    using std::map;
    using std::tuple;
    using std::chrono::microseconds;

    using nlohmann::json;

    class CommandControlConsoleUi : public ICommandControlAcceptor
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

    protected:
        IModelRunner& modelRunner_;
        const IModelHelper* helper_;

    public:
        CommandControlConsoleUi(IModelRunner& modelRunner) :
            modelRunner_(modelRunner),
            helper_(modelRunner.Helper())
        {
        }

        // ICommandControlAcceptor implementation.
    public:
        virtual const string& Description() override
        {
            static string description { "Console UI" };
            return description;
        }

        virtual bool ParseArguments(int argc, char* argv[]) override
        {
            cout << "Console UI C/C initializing\n";

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

            return true;
        }

        virtual bool Initialize() override
        {
            width_ = helper_->Width();
            height_ = helper_->Height();

            if (windowWidth_ > width_) windowWidth_ = width_;
            if (windowHeight_ > height_) windowHeight_ = height_;

            centerHeight_ = ceil(windowHeight_ / 2);
            centerWidth_ = ceil(width_ / 2);

            LoadOptionalNamedNeurons();

            return true;
        }

        virtual bool AcceptAndExecute(unique_ptr<IQueryHandler> const & queryHandler) override
        {
            constexpr char KEY_UP = 'A';
            constexpr char KEY_DOWN = 'B';
            constexpr char KEY_LEFT = 'D';
            constexpr char KEY_RIGHT = 'C';

            KeyListener listener;

            bool quit {false};

            if (displayOn_)
            {
                if (displayMonitoredNeurons_)
                    PrintMonitoredNeurons();
                else
                    PrintNetworkScan();
            }

            char c {' '};
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
                        json periodQuery = { {"query","control"}, {"values", {{"engineperiod", newPeriod.count()}}} };
                        queryHandler->HandleQuery(periodQuery.dump());
                        break;
                    }

                    case '-':
                    {
                        auto newPeriod = modelRunner_.EnginePeriod() * 10;
                        if (newPeriod > microseconds(10'000'000)) newPeriod = microseconds(10'000'000);
                        json periodQuery = { {"query","control"}, {"values", {{"engineperiod", newPeriod.count()}}} };
                        queryHandler->HandleQuery(periodQuery.dump());
                        break;
                    }

                    case 'p':
                    case 'P':
                    {
                        json periodQuery = { {"query","control"}, {"values", {{"pause", true}}} };
                        queryHandler->HandleQuery(periodQuery.dump());
                        break;
                    }

                    case 'r':
                    case 'R':
                    {
                        json periodQuery = { {"query","control"}, {"values", {{"pause", false}}} };
                        queryHandler->HandleQuery(periodQuery.dump());
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

            return quit;
        }

    protected:
        virtual const string& Legend() = 0;
        virtual char EmitToken(unsigned long neuronIndex) = 0;

    private:
        void LoadOptionalNamedNeurons()
        {
            const json& configuration = modelRunner_.Configuration();
            if (configuration.contains("Model"))
            {
                const auto& modelSection = configuration["Model"];
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
                if (neuronIndex > modelRunner_.ModelSize()) neuronIndex = 0;
            }

            cout
                <<  Legend() << ":(" << centerWidth_ << "," << centerHeight_ << ") "
                << " Tick: " << modelRunner_.EnginePeriod().count() << " us "
                << "Iterations: " << modelRunner_.GetIterations() 
                << "  Total work: " << modelRunner_.GetTotalWork() 
                << "                 \n";

            cout << "Arrow keys to navigate       + and - keys control speed            q to quit\n";
        }

        void PrintMonitoredNeurons()
        {
            cout << cls;

            for (auto& [neuronName, posTuple] : namedNeurons_)
            {
                auto& [ypos, xpos] = posTuple;
                auto neuronIndex = helper_->GetIndex(ypos, xpos);
                auto neuronActivation = helper_->GetNeuronActivation(neuronIndex);
                auto neuronTicksSinceLastSpike = helper_->GetNeuronTicksSinceLastSpike(neuronIndex);
                cout << "Neuron " << std::setw(15) << neuronName << " [" << ypos << ", " << xpos << "] = " << std::setw(4) << neuronIndex << ": ";
                cout << std::setw(5) << neuronActivation << "(" << std::setw(3) << neuronTicksSinceLastSpike << ")";
                cout << std::endl;

                for (auto synapseId = 0; synapseId < SynapticConnectionsPerNode; synapseId++)
                {
                    if (helper_->IsSynapseUsed(neuronIndex, synapseId))
                    {
                        auto presynapticNeuronIndex = helper_->GetPresynapticNeuron(neuronIndex, synapseId);
                        cout << std::setw(20) << presynapticNeuronIndex
                        << "(" << std::setw(3) << helper_->GetSynapticStrength(neuronIndex, synapseId) << ")  ";
                    }
                }

                cout << std::endl;
                cout << std::endl;
            }
            cout << std::endl;

            cout
                <<  Legend() << ": "
                << " Tick: " << modelRunner_.EnginePeriod().count() << " us "
                << "Iterations: " << modelRunner_.GetIterations() 
                << "  Total work: " << modelRunner_.GetTotalWork() 
                << "                 \n";

            cout << "+ and - keys control speed            q to quit\n";
        }
    };
}
