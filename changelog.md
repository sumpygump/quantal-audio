# Quantal Audio VCV Rack Modules | Changelog

## 2.2.2 (2025-02-14)

 - Fix bug with smooth level CV to support 16-channel polyphony

## 2.2.1 (2025-02-13)

 - Add ability to solo a Daisy channel by long-holding the mute button
 - Add keyboard shortcuts 'm' and 's' to mute and solo Daisy channel
 - Add context menu option to Daisy channel to make 'Direct outs pre-mute'
 - Add context menu option to Daisy channel, Daisy master, Mixer-2 to make
   'Smooth level CV' with 6ms slew
 - Slight optimization in VU meter light calculation
 - Add channel strip labels to Daisy channel strips: will be labeled 1-16 when connected

## 2.2.0 (2025-02-02)

 - Add support for dark theme versions of all modules
 - Add context menu commands in Daisy master to spawn channel strips
 - New module: Daisy blank, a chainable module within Daisy mixer modules
 - Update Daisy aux module to support grouping of signals in groups 1 or 2.
   Updates mechanism to send to aux groups using new context menu sliders in
   Daisy channel strip module: can send this channel to group 1 or 2 aux.
 - Deprecate original Daisy mixer modules (mono v1 with wired approach)

## 2.1.3 (2023-03-06)

 - Fix bug with noise floor present in Daisy chainged signals; was caused by
   expander messages not being thread-safe.

## 2.1.2 (2023-03-03)

 - New module: Daisy VU meter. As an expander module to Daisy channel, Daisy
   aux and Daisy master modules.

## 2.1.0 (2023-03-01)

 - New module: Daisy aux. For collecting signals from previous Daisy channels
   and sending to an 'outboard' effect. Can return signal to another Daisy
   channel in mixer chain.
 - Add panning parameter knob to Daisy channel module
 - New modules: Daisy channel (stereo), Daisy mixer; uses expander technology
   to daisy chain mixer signals.

## 2.0.1 (2021-11-20)
 
 - Add polyphony support and improve Horsehair performance
 - Update license

## 2.0.0 (2021-10-23)

 - Upgrade to Rack 2.0

## 1.0.0 (2019-06-29)

 - Upgrade to Rack 1.0

## 0.6.4 (2019-01-01)
 
 - New module: Horsehair VCO
 - Fix bugs in Unity mix

## 0.6.3 (2018-12-19)

 - New modules: Daisy channel (mono v1) and Daisy master (v1)

## 0.6.1 (2018-11-27)

 - New module: Buffered multiple and Unity mix
 - Standardize module naming conventions

## 0.6.0 (2018-11-24)

 - New modules: Blank1, Blank3, Blank5, Mixer-2
