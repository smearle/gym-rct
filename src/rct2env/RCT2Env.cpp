/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../openrct2-ui/Ui.h"

#include "UiContext.h"
#include "../openrct2-ui/audio/AudioContext.h"
#include "../openrct2-ui/drawing/BitmapReader.h"

#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/cmdline/CommandLine.hpp>
#include <openrct2/platform/platform.h>
#include <openrct2/ui/UiContext.h>

#include <cpprl/cpprl.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <ATen/Parallel.h>
#include <torch/torch.h>

#include "Env.h"

using namespace OpenRCT2;
using namespace OpenRCT2::Audio;
using namespace OpenRCT2::Ui;
using namespace cpprl;

// Algorithm hyperparameters
const std::string algorithm = "PPO";
const float actor_loss_coef = 1.0;
const int batch_size = 2;
const float clip_param = 0.2;
const float discount_factor = 0.99;
const float entropy_coef = 1e-3;
const float gae = 0.9;
const float kl_target = 0.5;
const float learning_rate = 1e-3;
const int log_interval = 10;
const int max_frames = 10e+7;
const int num_epoch = 3;
const int num_mini_batch = 2;
const int reward_average_window_size = 10;
const float reward_clip_value = 100; // Post scaling
const bool use_gae = true;
const bool use_lr_decay = false;
const float value_loss_coef = 0.5;

// Environment hyperparameters
const std::string env_name = "RCT2Env-v0";
const int num_envs = 1;
const float render_reward_threshold = 160;

// Model hyperparameters
const int hidden_size = 64;
const bool recurrent = false;
const bool use_cuda = false;

std::vector<float> flatten_vector(std::vector<float> const &input)
{
    return input;
}

template <typename T>
std::vector<float> flatten_vector(std::vector<std::vector<T>> const &input)
{
    std::vector<float> output;

    for (auto const &element : input)
    {
        auto sub_vector = flatten_vector(element);

        output.reserve(output.size() + sub_vector.size());
        output.insert(output.end(), sub_vector.cbegin(), sub_vector.cend());
    }

    return output;
}

template<typename T> static std::shared_ptr<T> to_shared(std::unique_ptr<T>&& src)
{
    return std::shared_ptr<T>(std::move(src));
}


RCT2Env make_env(int argc, const char** argv, Agent* agent)
{
    RCT2Env env;
    env.Init(argc, argv, agent);
////RCT2Env env2;
////printf("launching second env");
////env2.Init(argc, argv, agent);
////printf("launching second env");
    return env;
}

#ifdef __ANDROID__
extern "C" {
int SDL_main(int argc, const char* argv[])
{
    return main(argc, argv);
}
}
#endif

int train(int argc, const char **argv) {

    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("%^[%T %7l] %v%$");

    at::set_num_threads(1);
    torch::manual_seed(27);

    torch::Device device = use_cuda ? torch::kCUDA : torch::kCPU;

    spdlog::info("Launching an RCT2 env");
    Agent agent = Agent();
    RCT2Env env = make_env(argc, argv, &agent); 

    auto env_info = env.GetInfo();
  //spdlog::info("Action space: {} - [{}]", env_info->action_space_type,
  //             env_info->action_space_shape);
  //spdlog::info("Observation space: {} - [{}]", env_info->observation_space_type,
  //             env_info->observation_space_shape);

    spdlog::info("Resetting environment");
//  auto reset_param = std::make_shared<ResetParam>();
//  Request<ResetParam> reset_request("reset", reset_param);
//  communicator.send_request(reset_request);
	torch::Tensor observation;
    observation = env.Reset();
//  while (true) {
//      //spdlog::info(screenshot());
//      env.Step();
//  }

    auto observation_shape = env_info->observation_space_shape;
    observation_shape.insert(observation_shape.begin(), num_envs);
  //if (env_info->observation_space_shape.size() > 1)
  //{
  //  //observation_vec = flatten_vector(communicator.get_response<CnnResetResponse>()->observation);
  //    observation = torch::from_blob(observation_vec.data(), observation_shape).to(device);
  //}
  //else
  //{
  //  //observation_vec = flatten_vector(communicator.get_response<MlpResetResponse>()->observation);
  //    observation = torch::from_blob(observation_vec.data(), observation_shape).to(device);
  //}
  //

    std::shared_ptr<NNBase> base;
    if (env_info->observation_space_shape.size() == 1)
    {
        base = std::make_shared<MlpBase>(env_info->observation_space_shape[0], recurrent, hidden_size);
    }
    else
    {
        base = std::make_shared<CnnBase>(env_info->observation_space_shape[0], recurrent, hidden_size);
    }
    base->to(device);

    ActionSpace space{env_info->action_space_type, env_info->action_space_shape};
    Policy policy(nullptr);
    if (env_info->observation_space_shape.size() == 1)
    {
        // With observation normalization
        policy = Policy(space, base, true);
    }
    else
    {
        // Without observation normalization
        policy = Policy(space, base, false);
    }
    policy->to(device);
    RolloutStorage storage(batch_size, num_envs, env_info->observation_space_shape, space, hidden_size, device);
    std::unique_ptr<Algorithm> algo;
    if (algorithm == "A2C")
    {
        algo = std::make_unique<A2C>(policy, actor_loss_coef, value_loss_coef, entropy_coef, learning_rate);
    }
    else if (algorithm == "PPO")
    {
        algo = std::make_unique<PPO>(policy,
                                     clip_param,
                                     num_epoch,
                                     num_mini_batch,
                                     actor_loss_coef,
                                     value_loss_coef,
                                     entropy_coef,
                                     learning_rate,
                                     1e-8,
                                     0.5,
                                     kl_target);
    }

    storage.set_first_observation(observation);

    std::vector<float> running_rewards(num_envs);
    int episode_count = 0;
    bool render = false;
    std::vector<float> reward_history(reward_average_window_size);
    RunningMeanStd returns_rms(1);
    auto returns = torch::zeros({num_envs});

    auto start_time = std::chrono::high_resolution_clock::now();

    int num_updates = max_frames / (batch_size * num_envs);
    for (int update = 0; update < num_updates; ++update)
    {
        for (int step = 0; step < batch_size; ++step)
        {
            std::vector<torch::Tensor> act_result;
            {
 
                torch::NoGradGuard no_grad;
              //std::cout << "getting action from policy" << std::endl;
                act_result = policy->act(storage.get_observations()[step],
                                         storage.get_hidden_states()[step],
                                         storage.get_masks()[step]);
              //std::cout << "got action from policy" << std::endl;
            }
            auto actions_tensor_raw = act_result[1].cpu();
//          std::cout << actions_tensor_raw << std::endl;
            auto actions_tensor = act_result[1].cpu().to(torch::kBool);
            bool *actions_array = actions_tensor.data_ptr<bool>();
          //std::cout << actions_array << std::endl;
            std::vector<std::vector<bool>> actions(num_envs);
          //std::cout << actions_tensor.sizes() << std::endl;
            for (int i = 0; i < num_envs; ++i)
            {
                if (space.type == "Discrete")
                {
                    actions[i] = {actions_array[i]};
                }
                else
                {
                    for (int j = 0; j < env_info->action_space_shape[0]; j++)
                    {
                        actions[i].push_back(actions_array[i * env_info->action_space_shape[0] + j]);
                    }
                }
            }

          //auto step_param = std::make_shared<StepParam>();
          //step_param->actions = actions;
          //step_param->render = render;
          //Request<StepParam> step_request("step", step_param);
          //communicator.send_request(step_request);
			StepResult step_result = env.Step(actions);
			torch::Tensor rewards = step_result.rewards;
	//std::cout << rewards << std::endl;
			torch::Tensor real_rewards = rewards.clone();
	//torch::Tensor real_rewards = step_result->rewards;
          //std::vector<float> real_rewards->real_rewards;
          //std::vector<std::vector<bool>> dones_vec;
			std::vector<std:: vector<bool> > dones_vec = step_result.done;
		////std::cout << step_result.done << std::endl;
		////std::cout << dones_vec << std::endl;
			observation = step_result.observation;
            if (env_info->observation_space_shape.size() > 1)
            {
              //auto step_result = communicator.get_response<CnnStepResponse>();
              //observation_vec = flatten_vector(step_result->observation);
              //observation = torch::from_blob(observation_vec.data(), observation_shape).to(device);
              //auto raw_reward_vec = flatten_vector(step_result->real_reward);
              //auto reward_tensor = torch::from_blob(raw_reward_vec.data(), {num_envs}, torch::kFloat);
                returns = returns * discount_factor + rewards;
                returns_rms->update(returns);
                rewards = torch::clamp(rewards / torch::sqrt(returns_rms->get_variance() + 1e-8),
                                             -reward_clip_value, reward_clip_value);
              //rewards = std::vector<float>(reward_tensor.data_ptr<float>(), reward_tensor.data_ptr<float>() + reward_tensor.numel());
              //real_rewards = flatten_vector(step_result->real_reward);
                dones_vec = step_result.done;
            }
            else
            {
              //auto step_result = communicator.get_response<MlpStepResponse>();
              //observation_vec = flatten_vector(step_result->observation);
              //observation = torch::from_blob(observation_vec.data(), observation_shape).to(device);
              //auto raw_reward_vec = flatten_vector(step_result->real_reward);
              //auto reward_tensor = torch::from_blob(raw_reward_vec.data(), {num_envs}, torch::kFloat);
                returns = returns * discount_factor + rewards;
                returns_rms->update(returns);
                rewards = torch::clamp(rewards / torch::sqrt(returns_rms->get_variance() + 1e-8),
                                             -reward_clip_value, reward_clip_value);
              //rewards = std::vector<float>(reward_tensor.data_ptr<float>(), reward_tensor.data_ptr<float>() + reward_tensor.numel());
              //real_rewards = flatten_vector(step_result->real_reward);
                dones_vec = step_result.done;
            }
            for (int i = 0; i < num_envs; ++i)
            {
              //running_rewards[i] += real_rewards[i];
                running_rewards[i] += real_rewards[i].item<float>();
                if (dones_vec[i][0])
                {
                    reward_history[episode_count % reward_average_window_size] = running_rewards[i];
                    running_rewards[i] = 0;
                    returns[i] = 0;
                    episode_count++;
                }
            }
            auto dones = torch::zeros({num_envs, 1}, TensorOptions(device));
            for (int i = 0; i < num_envs; ++i)
            {
                dones[i][0] = static_cast<int>(dones_vec[i][0]);
            }

            storage.insert(observation,
                           act_result[3],
                           act_result[1],
                           act_result[2],
                           act_result[0],
                         //torch::from_blob(rewards.data(), {num_envs, 1}).to(device),
						   rewards,
                           1 - dones);
        }


        torch::Tensor next_value;
        {
            torch::NoGradGuard no_grad;
            next_value = policy->get_values(
                                   storage.get_observations()[-1],
                                   storage.get_hidden_states()[-1],
                                   storage.get_masks()[-1])
                             .detach();
        }
        storage.compute_returns(next_value, use_gae, discount_factor, gae);

        float decay_level;
        if (use_lr_decay)
        {
            decay_level = 1. - static_cast<float>(update) / num_updates;
        }
        else
        {
            decay_level = 1;
        }
        auto update_data = algo->update(storage, decay_level);
        storage.after_update();

        if (update % log_interval == 0 && update > 0)
        {
            auto total_steps = (update + 1) * batch_size * num_envs;
            auto run_time = std::chrono::high_resolution_clock::now() - start_time;
            auto run_time_secs = std::chrono::duration_cast<std::chrono::seconds>(run_time);
            auto fps = total_steps / (run_time_secs.count() + 1e-9);
		////std::cout << reward_history << std::endl;
            spdlog::info("---");
            spdlog::info("Update: {}/{}", update, num_updates);
            spdlog::info("Total frames: {}", total_steps);
            spdlog::info("FPS: {}", fps);
            for (const auto &datum : update_data)
            {
                spdlog::info("{}: {}", datum.name, datum.value);
            }
            float average_reward = std::accumulate(reward_history.begin(), reward_history.end(), 0);
            average_reward /= episode_count < reward_average_window_size ? episode_count : reward_average_window_size;
            spdlog::info("Reward: {}", average_reward);
            render = average_reward >= render_reward_threshold;
        }
    }
}

/**
 * Main entry point for non-Windows systems. Windows instead uses its own DLL proxy.
 */
#if defined(_MSC_VER) && !defined(__DISABLE_DLL_PROXY__)
int NormalisedMain(int argc, const char** argv)
#else
int main(int argc, const char** argv)
#endif
{
    return train(argc, argv);
}


