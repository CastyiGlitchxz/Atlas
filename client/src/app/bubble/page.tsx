'use client'
import React from 'react';
import styles from "../../stylesheets/css/bubble.module.css";

export default function Bubble() {
    return (
        <div className={styles.bubble_root}>
            <div className={styles.bubble_pms_list}>
                <strong>Your Messages</strong>

                <div>

                </div>
            </div>

            <div>
                <span>
                    <p>Welcome to your personal bubble!</p>
                </span>

                <div>
                    <strong>Friends</strong>
                    <div>
                        <div className={styles.bubble_friend}>
                            <div className={styles.bubble_friend_userinfo}>
                                <img />
                                <div>
                                    <strong>Kaguya Hime</strong>
                                    <p>Princess of the moon, KH.</p>
                                </div>
                            </div>

                            <div className={styles.bubble_friend_options}>
                                <button>C</button>
                                <button>M</button>
                                <button>O</button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}