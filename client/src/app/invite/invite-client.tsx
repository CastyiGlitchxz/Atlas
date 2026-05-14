'use client'
import React, { useEffect, useState } from "react";
import { eventManager } from "../../typescript/eventsManager";
import { getWebSocket } from "../../typescript/websocket";
import { useRouter, useSearchParams } from "next/navigation";
import { get_token } from "../../typescript/user";
import styles from "../../stylesheets/css/invite.module.css";
import { globals } from "../../typescript/env";
import Image from "next/image";

export default function Invite() {
    const searchParams = useSearchParams();
    const code = searchParams.get("code");
    const router = useRouter();

    const em = new eventManager();

    const [server, setServer] = useState<{
        issuer: string,
        server: {
            sid: string,
            server_name: string,
            icon: string | null
        },
    }>();
    
    useEffect(() => {
        const ws = getWebSocket();
        ws.onopen = () => {
            em.emitEvent("verify_invite", { "code": code });
        };

        ws.onmessage = (msg) => {
            const {event, data} = JSON.parse(msg.data);

            switch(event) {
                case "invite":
                    if (!data.failed) {
                        setServer({
                            issuer: data.issued_by,
                            server: {
                                sid: data.server.sid,
                                server_name: data.server.server_name,
                                icon: data.server.icon
                            }
                        });
                        console.log(data)
                    } else {
                        router.replace(`/invite/invalid_code`);
                    }
                    break;

                case "server_response":
                    if (data.server.response === "success") {
                        router.replace("/bubble");
                    }
                    break;

                default:
                    break;
            }
        }
        return () => {
            ws.onmessage = null;
            ws.onopen = null;
        };
    }, []);

    function join_server() {
        const token = get_token();
        em.emitEvent("join_server", { "token": token, sid: server?.server.sid  });
    }

    return (
        <div className={styles.main}>
            <div className={styles.topLink}>
                <p>{globals.url_string.scheme}://{globals.url_string.subdomain}/invite?code={code}</p>
            </div>
            <div className={styles.invite_prompt}>
                <div className={styles.serverIcon}>
                    {server && server.server.icon !== null ?
                        <Image src={`${globals.url_string.scheme}://${globals.url_string.subdomain}/${server.server.icon}`} alt="" width={20} height={20} unoptimized className={styles.serverIconPicture} draggable={false}/>
                        :
                        <p>{server?.server.server_name[0]}</p>
                    }
                </div>
                <p><b>{server?.issuer}</b> invited you to <b>{server?.server.server_name}</b></p>
                <button onClick={() => join_server()} className={styles.joinButton}>Join Server</button>
            </div>
        </div>
    )
}