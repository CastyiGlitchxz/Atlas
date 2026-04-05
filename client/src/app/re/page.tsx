'use client'
import React, { useRef, useState } from 'react';

export default function VoiceChat() {
  const remoteAudioRef = useRef(null);
  const pc = useRef(null);
  const [status, setStatus] = useState("Disconnected");

  const joinChat = async () => {
    setStatus("Initializing...");
    
    // 1. Setup PeerConnection
    pc.current = new RTCPeerConnection({
      iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
    });

    // Monitor connection state directly from the browser's perspective
    pc.current.onconnectionstatechange = () => {
      console.log("WebRTC State Change:", pc.current.connectionState);
      if (pc.current.connectionState === 'connected') {
        setStatus("Network Connected (Waiting for Voice...)");
      }
    };

    // This fires when Python relays someone else's audio to you
    pc.current.ontrack = (event) => {
      console.log("Remote track received!", event.streams[0]);
      if (remoteAudioRef.current) {
        remoteAudioRef.current.srcObject = event.streams[0];
        setStatus("Connected - Audio Live");
      }
    };

    try {
      // 2. Get Microphone
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      stream.getTracks().forEach(track => pc.current.addTrack(track, stream));

      // 3. Create Offer
      const offer = await pc.current.createOffer();
      await pc.current.setLocalDescription(offer);

      // 4. WAIT FOR ICE GATHERING (Crucial for Tailscale/Nginx)
      // This ensures the SDP sent to Python isn't "empty" of network paths
      await new Promise((resolve) => {
        if (pc.current.iceGatheringState === 'complete') {
          resolve();
        } else {
          const checkState = () => {
            if (pc.current.iceGatheringState === 'complete') {
              pc.current.removeEventListener('icegatheringstatechange', checkState);
              resolve();
            }
          };
          pc.current.addEventListener('icegatheringstatechange', checkState);
        }
      });

      // 5. Send to Python
      setStatus("Exchanging Signaling...");
      const response = await fetch('https://coffeecinema.tail7bc346.ts.net/offer', {
        method: 'POST',
        body: JSON.stringify({ 
          sdp: pc.current.localDescription.sdp, 
          type: pc.current.localDescription.type 
        }),
        headers: { 'Content-Type': 'application/json' }
      });

      const answer = await response.json();
      await pc.current.setRemoteDescription(new RTCSessionDescription(answer));
      
      setStatus("Negotiation Complete");

    } catch (e) {
      console.error("WebRTC Error:", e);
      setStatus("Error: " + e.message);
    }
  };

  return (
    <div style={{ padding: '40px', textAlign: 'center', fontFamily: 'sans-serif' }}>
      <h2>Voice Chat Room</h2>
      <div style={{ 
        display: 'inline-block', 
        padding: '10px 20px', 
        borderRadius: '20px', 
        backgroundColor: status.includes('Live') ? '#d4edda' : '#eee',
        color: status.includes('Live') ? '#155724' : '#333',
        marginBottom: '20px'
      }}>
        Status: <strong>{status}</strong>
      </div>
      
      <br />
      
      {/* Show controls temporarily to verify the audio stream is active */}
      <audio ref={remoteAudioRef} autoPlay playsInline controls style={{ display: 'block', margin: '20px auto' }} />

      <button 
        onClick={joinChat} 
        style={{ 
          padding: '12px 24px', 
          fontSize: '16px', 
          cursor: 'pointer',
          backgroundColor: '#007bff',
          color: 'white',
          border: 'none',
          borderRadius: '5px'
        }}
      >
        Join Voice Channel
      </button>
    </div>
  );
}