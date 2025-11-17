import React from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import styles from './index.module.css';

function HomepageHeader() {
    const {siteConfig} = useDocusaurusContext();
    return (
        <header className={clsx('hero hero--primary', styles.heroBanner)}>
            <div className="container">
                <h1 className="hero__title">{siteConfig.title}</h1>
                <p className="hero__subtitle">{siteConfig.tagline}</p>
                <div className={styles.buttons}>
                    <Link
                        className="button button--secondary button--lg"
                        to="/docs/quick-start">
                        Get Started - 5min ⏱️
                    </Link>
                </div>
            </div>
        </header>
    );
}

export default function Home() {
    const {siteConfig} = useDocusaurusContext();
    return (
        <Layout
            title={`${siteConfig.title}`}
            description="ESP32-C3 powered public transport departure board documentation">
            <HomepageHeader/>
            <main>
                <div className="container" style={{marginTop: '2rem', marginBottom: '2rem'}}>
                    <div className="row">
                        <div className="col col--4">
                            <h3>🚀 Quick Start</h3>
                            <p>
                                Get your MyStation up and running in minutes with our comprehensive
                                quick start guide.
                            </p>
                        </div>
                        <div className="col col--4">
                            <h3>🔧 Easy Configuration</h3>
                            <p>
                                Configure your e-paper display board with a simple web interface
                                for weather and transport data.
                            </p>
                        </div>
                        <div className="col col--4">
                            <h3>⚡ ESP32-C3 Powered</h3>
                            <p>
                                Low power consumption with deep sleep support, perfect for
                                battery-powered deployments.
                            </p>
                        </div>
                    </div>
                </div>
            </main>
        </Layout>
    );
}
