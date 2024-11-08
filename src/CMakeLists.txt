add_library(jucemodules STATIC)

target_link_libraries(jucemodules PUBLIC
	juce::juce_audio_formats
	juce::juce_audio_devices
	juce::juce_audio_utils
	juce::juce_events
)

target_compile_definitions(jucemodules
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_MODAL_LOOPS_PERMITTED=1
)

add_subdirectory(hearingtest)