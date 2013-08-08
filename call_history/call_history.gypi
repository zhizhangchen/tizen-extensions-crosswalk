{
  'targets': [
    {
      'target_name': 'tizen_call_history',
      'type': 'loadable_module',
      'sources': [
        'call_history_api.js',
        'call_history_context.cc',
        'call_history_context.h',
        'call_history_context_desktop.cc',
        'call_history_context_mobile.cc',
      ],
      'dependencies': [
        'wrt-plugins-tizen-callhistory',
        'vconf',
      ],
    },

    {
      'target_name': 'wrt-plugins-tizen-callhistory',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags wrt-plugins-tizen-callhistory)',
              '<!@(pkg-config --cflags dpl-efl)',
              '<!@(pkg-config --cflags wrt-plugins-commons)',
              '<!@(pkg-config --cflags wrt-plugins-tizen-tizen)',
              '-std=c++0x -DDPL_LOGS_ENABLED'
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other wrt-plugins-tizen-callhistory)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l wrt-plugins-tizen-callhistory)',
              '-lwrt-plugins-tizen-callhistory',
            ],
          }
        }],
      ],
    },
  ],
}
