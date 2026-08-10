'use client'
import React from "react";
import styles from "../../../../stylesheets/css/chat.module.css";

export default function Chat({ params }: { params: Promise<{ serverID: string }> }) {
    return (
        <div className={styles.welcome}>

            <div className={styles["rules-panel"]}>
                <p>
                    <strong>Rules</strong>
                </p>
                <textarea readOnly></textarea>
            </div>
        </div>
    )
}