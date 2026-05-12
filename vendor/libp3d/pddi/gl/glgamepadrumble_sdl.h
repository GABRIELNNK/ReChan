#pragma once

bool glGamepadPollState(bool outButtons[15], float outAxes[6]);
bool glGamepadRumbleUpdateActive(int glfwGamepadIndex, const char* glfwGuid);
bool glGamepadRumbleSupports();
bool glGamepadRumbleSet(float lowFrequency, float highFrequency);
