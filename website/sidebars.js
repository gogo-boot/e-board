/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */

// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
    // By default, Docusaurus generates a sidebar from the docs folder structure
    tutorialSidebar: [
        {
            type: 'category',
            label: 'Getting Started',
            items: [
                'README',
                'quick-start',
                'hardware-setup',
                'software-setup',
            ],
        },
        {
            type: 'category',
            label: 'Configuration',
            items: [
                'configuration',
                'configuration-data-flow',
                'configuration-flowchart',
                'configuration-keys-mapping',
                'configuration-keys-quick-reference',
                'configuration-phase-quick-reference',
                'configuration-phase-tracking',
                'ota-configuration-feature',
            ],
        },
        {
            type: 'category',
            label: 'Display',
            items: [
                'display-layout-overview',
                'e-paper-config-instructions',
                'e-paper-library-comparison',
                'partial-update-bug-analysis',
            ],
        },
        {
            type: 'category',
            label: 'Features',
            items: [
                'button-quick-reference',
                'button-temporary-mode-feature',
                'button-wakeup-debugging',
            ],
        },
        {
            type: 'category',
            label: 'API & Integration',
            items: [
                'api-keys',
            ],
        },
        {
            type: 'category',
            label: 'Development',
            items: [
                'native-testing-setup',
                'mock-time-implementation',
                'test-rtc-timestamps',
            ],
        },
        {
            type: 'category',
            label: 'Implementation Details',
            items: [
                'dedicated-german-phase-functions',
                'final-fix-setdefaults-bug',
                'main-refactoring-summary',
                'ota-integration-summary',
                'ota-sleep-timing-integration',
                'phase-transition-bugfix',
                'phase-transition-implementation-summary',
                'phase1-to-phase2-transition',
                'phase2-infinite-restart-fix',
                'refresh-process',
                'wifi-config-refactoring-summary',
            ],
        },
    ],
};

export default sidebars;

