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
        #'jsc-wrapper',
        #'vconf',
      ],
    },
    {
      'target_name': 'jsc-wrapper',
      'type': 'loadable_module',
      'sources': [
        'jsc_wrapper.cc',
      ],
      'dependencies': [
        'ewebkit2',
      ],
      'conditions': [
        ['type=="mobile"', {
          'link_settings': {
            #'ldflags': [
            #  '-Lout/Default'
            #],
            #'libraries': [
            #  '-ljsc-wrapper'
            #],
          }
        }],
      ],
    },
    {
      'target_name': 'ewebkit2',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags ewebkit2)',
              '<!@(pkg-config --cflags wrt-plugins-commons)',
              '<!@(pkg-config --cflags wrt-plugins-types)',
              '<!@(pkg-config --cflags security-client)',
              '-std=c++0x -DDPL_LOGS_ENABLED'
            ],
          },
          'link_settings': {
            'libraries': [
              '-ldl'
            ],
          }
        }],
      ],
    },

    {
      'target_name': 'wrt-plugins-tizen-callhistory',
      'type': 'none',
      'conditions': [
        ['type=="mobile"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags wrt-plugin-loading)',
              '<!@(pkg-config --cflags wrt-plugins-types)',
              '-std=c++0x -DDPL_LOGS_ENABLED'
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other wrt-plugin-loading)',
            ],
            'libraries': [
              #'<!@(pkg-config --libs-only-l wrt-plugin-loading)',
            ],
          }
        }],
      ],
    },
  ],
}
