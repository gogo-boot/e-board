// @ts-check
// `@type` JSDoc annotations allow editor autocompletion and type checking
// (when paired with `@ts-check`).
// There are various equivalent ways to declare your Docusaurus config.
// See: https://docusaurus.io/docs/api/docusaurus-config

import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
    title: 'MyStation Documentation',
    tagline: 'ESP32-C3 powered public transport departure board',
    favicon: 'img/favicon.ico',

    // Set the production url of your site here
    url: 'https://gogo-boot.github.io',
    // Set the /<baseUrl>/ pathname under which your site is served
    // For GitHub pages deployment, it is often '/<projectName>/'
    baseUrl: '/e-board/',

    // GitHub pages deployment config.
    // If you aren't using GitHub pages, you don't need these.
    organizationName: 'gogo-boot', // Usually your GitHub org/user name.
    projectName: 'e-board', // Usually your repo name.
    deploymentBranch: 'gh-pages',
    trailingSlash: false,

    onBrokenLinks: 'warn',
    onBrokenMarkdownLinks: 'warn',

    // Even if you don't use internationalization, you can use this field to set
    // useful metadata like html lang. For example, if your site is Chinese, you
    // may want to replace "en" with "zh-Hans".
    i18n: {
        defaultLocale: 'en',
        locales: ['en'],
    },

    presets: [
        [
            'classic',
            /** @type {import('@docusaurus/preset-classic').Options} */
            ({
                docs: {
                    path: '../docs',
                    routeBasePath: 'docs',
                    sidebarPath: './sidebars.js',
                    // Please change this to your repo.
                    // Remove this to remove the "edit this page" links.
                    editUrl:
                        'https://github.com/YOUR-USERNAME/mystation/tree/main/',
                },
                blog: false,
                theme: {
                    customCss: './src/css/custom.css',
                },
            }),
        ],
    ],

    themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
        ({
            // Replace with your project's social card
            image: 'img/docusaurus-social-card.jpg',
            navbar: {
                title: 'MyStation',
                logo: {
                    alt: 'MyStation Logo',
                    src: 'img/logo.svg',
                },
                items: [
                    {
                        type: 'docSidebar',
                        sidebarId: 'tutorialSidebar',
                        position: 'left',
                        label: 'Documentation',
                    },
                    {
                        href: 'https://github.com/YOUR-USERNAME/e-board',
                        label: 'GitHub',
                        position: 'right',
                    },
                ],
            },
            footer: {
                style: 'dark',
                links: [
                    {
                        title: 'Docs',
                        items: [
                            {
                                label: 'Quick Start',
                                to: '/docs/quick-start',
                            },
                            {
                                label: 'Hardware Setup',
                                to: '/docs/hardware-setup',
                            },
                            {
                                label: 'Software Setup',
                                to: '/docs/software-setup',
                            },
                        ],
                    },
                    {
                        title: 'More',
                        items: [
                            {
                                label: 'GitHub',
                                href: 'https://github.com/YOUR-USERNAME/e-board',
                            },
                        ],
                    },
                ],
                copyright: `Copyright © ${new Date().getFullYear()} MyStation Project. Built with Docusaurus.`,
            },
            prism: {
                theme: prismThemes.github,
                darkTheme: prismThemes.dracula,
                additionalLanguages: ['cpp', 'c', 'bash', 'json', 'yaml'],
            },
        }),
};

export default config;

