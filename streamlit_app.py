import streamlit as st
import subprocess
import tempfile
import os
from pathlib import Path

st.set_page_config(page_title="stegaSaur", page_icon="🦕", layout="centered")

st.markdown("""
    <style>
    .stApp {
        background-color: #1a1a1a;
        color: #ffffff;
    }
    </style>
""", unsafe_allow_html=True)

st.title("🦕 stegaSaur")
st.subheader("Steganography Educational Tool")

STEGASAUR_BINARY = Path(__file__).parent / 'stegasaur'

mode = st.radio("Choose Mode", ["Encode", "Decode"], horizontal=True)

if mode == "Encode":
    st.header("Encode Secret into Carrier")
    
    carrier_file = st.file_uploader(
        "Select Carrier File (PNG, JPEG, JPG)", 
        type=['png', 'jpg', 'jpeg'],
        key="carrier_encode"
    )
    
    secret_file = st.file_uploader(
        "Select Secret File (any type)", 
        key="secret"
    )
    
    if st.button("Encode", type="primary"):
        if carrier_file and secret_file:
            with tempfile.TemporaryDirectory() as tmpdir:
                carrier_path = Path(tmpdir) / f"carrier{Path(carrier_file.name).suffix}"
                secret_path = Path(tmpdir) / f"secret{Path(secret_file.name).suffix}"
                
                with open(carrier_path, 'wb') as f:
                    f.write(carrier_file.getbuffer())
                with open(secret_path, 'wb') as f:
                    f.write(secret_file.getbuffer())
                
                try:
                    process = subprocess.Popen(
                        [str(STEGASAUR_BINARY)],
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                        cwd=tmpdir
                    )
                    
                    input_data = f"{secret_path}\n{carrier_path}\n"
                    stdout, stderr = process.communicate(input=input_data, timeout=30)
                    
                    output_files = list(Path(tmpdir).glob('*.png'))
                    output_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
                    
                    if output_files:
                        encoded_file = output_files[0]
                        with open(encoded_file, 'rb') as f:
                            st.success("Encoding successful!")
                            st.download_button(
                                "Download Encoded File",
                                data=f.read(),
                                file_name="encoded.png",
                                mime="image/png"
                            )
                    else:
                        st.error(f"Encoding failed: {stderr}")
                        
                except Exception as e:
                    st.error(f"Error: {str(e)}")
        else:
            st.warning("Please select both carrier and secret files")

else:  # Decode
    st.header("Decode Secret from Carrier")
    
    encoded_file = st.file_uploader(
        "Select Encoded File (PNG)", 
        type=['png'],
        key="carrier_decode"
    )
    
    if st.button("Decode", type="primary"):
        if encoded_file:
            with tempfile.TemporaryDirectory() as tmpdir:
                encoded_path = Path(tmpdir) / "encoded.png"
                
                with open(encoded_path, 'wb') as f:
                    f.write(encoded_file.getbuffer())
                
                try:
                    process = subprocess.Popen(
                        [str(STEGASAUR_BINARY)],
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                        cwd=tmpdir
                    )
                    
                    input_data = f"{encoded_path}\n{encoded_path}\n"
                    stdout, stderr = process.communicate(input=input_data, timeout=30)
                    
                    decode_files = list(Path(tmpdir).glob('decode.*'))
                    if decode_files:
                        decoded_file = decode_files[0]
                        with open(decoded_file, 'rb') as f:
                            st.success("Decoding successful!")
                            st.download_button(
                                "Download Decoded File",
                                data=f.read(),
                                file_name=f"decoded{decoded_file.suffix}",
                                mime="application/octet-stream"
                            )
                    else:
                        st.error(f"Decoding failed: {stderr}")
                        
                except Exception as e:
                    st.error(f"Error: {str(e)}")
        else:
            st.warning("Please select an encoded file")

st.markdown("---")
st.markdown("**Note:** Currently supports PNG carriers and text/PNG secrets. JPEG/JPG and WAV support coming soon!")
