import streamlit as st
import subprocess
import tempfile
import os
from pathlib import Path
from PIL import Image
import io

st.set_page_config(page_title="stegaSaur Complete", page_icon="🦕", layout="centered")

st.title("🦕 stegaSaur")
st.subheader("Steganography Educational Tool")

# Determine if running on Windows and use WSL path
IS_WINDOWS = os.name == 'nt'

# Initialize session state for educational click-through
if 'edu_completed' not in st.session_state:
    st.session_state.edu_completed = {'png': False, 'jpeg': False, 'wav': False}
if 'edu_step' not in st.session_state:
    st.session_state.edu_step = 0
if 'edu_mode' not in st.session_state:
    st.session_state.edu_mode = None
if 'edu_file_type' not in st.session_state:
    st.session_state.edu_file_type = None
if 'pending_download' not in st.session_state:
    st.session_state.pending_download = None


def windows_to_wsl_path(windows_path):
    """Convert Windows path to WSL path format."""
    if not IS_WINDOWS:
        return windows_path
    
    path_str = str(windows_path)
    path_str = path_str.replace('\\', '/')
    
    if len(path_str) >= 2 and path_str[1] == ':':
        drive = path_str[0].lower()
        path_str = f'/mnt/{drive}{path_str[2:]}'
    
    return path_str


def calculate_carrier_capacity(uploaded_file):
    """Calculate the capacity of a carrier file."""
    if uploaded_file is None:
        return None, None

    file_ext = Path(uploaded_file.name).suffix.lower()
    
    try:
        uploaded_file.seek(0)
        file_data = uploaded_file.read()
        uploaded_file.seek(0)
        
        if file_ext == '.png':
            img = Image.open(io.BytesIO(file_data))
            width, height = img.size
            pixel_bytes = width * height * 4
            capacity_bytes = (pixel_bytes // 8) - 29
            
            return max(0, capacity_bytes), {
                'width': width,
                'height': height,
                'type': 'PNG (LSB)'
            }
            
        elif file_ext in ['.jpg', '.jpeg']:
            file_size = len(file_data)
            capacity_bytes = int(file_size * 0.10) - 25
            
            return max(0, capacity_bytes), {
                'file_size': file_size,
                'type': 'JPEG (DCT)',
                'note': 'Estimated capacity'
            }
            
        elif file_ext == '.wav':
            file_size = len(file_data)
            audio_data_size = max(0, file_size - 44)
            capacity_bytes = (audio_data_size // 8) - 29
            
            return max(0, capacity_bytes), {
                'file_size': file_size,
                'audio_data_size': audio_data_size,
                'type': 'WAV (LSB)'
            }
            
    except Exception as e:
        st.warning(f"Could not calculate capacity: {str(e)}")
        return None, None
    
    return None, None


def format_file_size(size_bytes):
    """Format file size in human-readable format."""
    if size_bytes < 1024:
        return f"{size_bytes} bytes"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.2f} MB"


def display_capacity_info(carrier_file, secret_file=None):
    """Display capacity information for carrier and secret files."""
    capacity_bytes, metadata = calculate_carrier_capacity(carrier_file)
    
    if capacity_bytes is None:
        return
    
    st.info(f"""
📊 **Carrier Capacity**

**Type:** {metadata['type']}  
**Capacity:** {format_file_size(capacity_bytes)}
""")
    
    if secret_file:
        secret_file.seek(0)
        secret_size = len(secret_file.read())
        secret_file.seek(0)
        
        usage_percent = (secret_size / capacity_bytes * 100) if capacity_bytes > 0 else 100
        
        st.write(f"**Secret Size:** {format_file_size(secret_size)}")
        st.write(f"**Usage:** {usage_percent:.1f}%")
        st.progress(min(usage_percent / 100, 1.0))
        
        if secret_size > capacity_bytes:
            st.error("❌ Secret file is too large for this carrier!")
        elif usage_percent > 90:
            st.warning(f"⚠️ Near capacity limit ({usage_percent:.1f}%)")
        else:
            st.success(f"✅ Good capacity usage ({usage_percent:.1f}%)")


def get_educational_steps(file_type, mode):
    """Get educational steps for click-through."""
    file_type = file_type.lower()
    
    if mode == "encode":
        if file_type == 'png':
            return [
                {
                    'title': 'Step 1: Understanding LSB Steganography',
                    'content': '''
**LSB (Least Significant Bit)** is a technique that hides data by modifying the least significant bit of each pixel's color values.

Since the LSB contributes minimally to the overall color (changing a value by ±1), the modifications are **imperceptible to the human eye**.

**Example:** RGB(255, 128, 64) → RGB(254, 129, 64)
'''
                },
                {
                    'title': 'Step 2: How PNG Encoding Works',
                    'content': '''
**The Process:**
1. Each pixel has 4 bytes (Red, Green, Blue, Alpha)
2. We modify the last bit of each byte to store 1 bit of secret data
3. An image with 1000×1000 pixels can hide ~500 KB of data

**Your secret data is being embedded bit-by-bit into the carrier image!**
'''
                },
                {
                    'title': 'Step 3: Encoding Complete!',
                    'content': '''
✅ **Your file has been successfully encoded!**

The secret data is now hidden inside the PNG image. The image looks identical to the original, but contains your hidden data.

**What happens next:**
- Download the encoded image
- Share it normally (email, cloud storage, etc.)
- Only someone with StegaSaur can extract the secret!
'''
                }
            ]
        elif file_type in ['jpeg', 'jpg']:
            return [
                {
                    'title': 'Step 1: Understanding DCT Steganography',
                    'content': '''
**DCT (Discrete Cosine Transform)** is used in JPEG compression. This technique hides data by modifying DCT coefficients in the frequency domain.

JPEG images are already compressed, so we work with the compression data itself rather than raw pixels.
'''
                },
                {
                    'title': 'Step 2: How JPEG Encoding Works',
                    'content': '''
**The Process:**
1. JPEG divides images into 8×8 pixel blocks
2. Each block is transformed into 64 DCT coefficients
3. We modify the LSB of non-zero, non-one coefficients
4. This preserves image quality while hiding data

**Note:** JPEG has lower capacity than PNG, best for text files.
'''
                },
                {
                    'title': 'Step 3: Encoding Complete!',
                    'content': '''
✅ **Your file has been successfully encoded!**

The secret data is now hidden in the JPEG's DCT coefficients. The image quality remains high and the changes are invisible.

**Advantage:** More robust to recompression than pixel-based methods!
'''
                }
            ]
        elif file_type == 'wav':
            return [
                {
                    'title': 'Step 1: Understanding Audio LSB',
                    'content': '''
**LSB for Audio** works similarly to images, but modifies audio sample values instead of pixel values.

Since the LSB contributes minimally to the amplitude, the modifications are **inaudible to the human ear**.
'''
                },
                {
                    'title': 'Step 2: How WAV Encoding Works',
                    'content': '''
**The Process:**
1. Each audio sample is represented by bytes
2. We modify the last bit of each byte to store 1 bit of secret data
3. A 1-minute WAV file can typically hide several megabytes

**The audio quality remains virtually identical to the original!**
'''
                },
                {
                    'title': 'Step 3: Encoding Complete!',
                    'content': '''
✅ **Your file has been successfully encoded!**

The secret data is now hidden in the audio samples. The audio sounds identical to the original.

**Perfect for:** Hiding data in music, podcasts, or any audio file!
'''
                }
            ]
    else:  # decode mode
        if file_type == 'png':
            return [
                {
                    'title': 'Step 1: LSB Extraction Process',
                    'content': '''
The decoder reverses the encoding process by reading the least significant bit from each byte of the carrier file.

These bits are reassembled into the original secret file.
'''
                },
                {
                    'title': 'Step 2: Verifying Data Integrity',
                    'content': '''
**The Process:**
1. Extract metadata (file type, size, dimensions)
2. **Verify checksum** to ensure data integrity
3. Extract the specified number of bits
4. Reconstruct the original file

If the checksum fails, the file may not contain hidden data or has been corrupted.
'''
                },
                {
                    'title': 'Step 3: Decoding Complete!',
                    'content': '''
✅ **Your secret file has been successfully extracted!**

The hidden data has been recovered and is ready to download. It's a perfect, lossless copy of the original file.
'''
                }
            ]
        elif file_type in ['jpeg', 'jpg']:
            return [
                {
                    'title': 'Step 1: DCT Coefficient Extraction',
                    'content': '''
The decoder reads DCT coefficients from the JPEG file and extracts the least significant bit from each non-zero, non-one coefficient.
'''
                },
                {
                    'title': 'Step 2: Reconstructing the Secret',
                    'content': '''
**The Process:**
1. Read JPEG DCT coefficients
2. Extract LSB from valid coefficients
3. Verify checksum for data integrity
4. Reconstruct the original file

This method is more resilient to JPEG recompression than pixel-based methods.
'''
                },
                {
                    'title': 'Step 3: Decoding Complete!',
                    'content': '''
✅ **Your secret file has been successfully extracted!**

The hidden data has been recovered from the JPEG's frequency domain. Ready to download!
'''
                }
            ]
        elif file_type == 'wav':
            return [
                {
                    'title': 'Step 1: Audio Sample Extraction',
                    'content': '''
The decoder reads the audio samples and extracts the least significant bit from each byte.

These bits are reassembled into the original secret file.
'''
                },
                {
                    'title': 'Step 2: Verifying Audio Data',
                    'content': '''
**The Process:**
1. Extract metadata from audio samples
2. Verify checksum to ensure integrity
3. Extract the hidden bits
4. Reconstruct the original file

The audio file itself remains playable and sounds normal!
'''
                },
                {
                    'title': 'Step 3: Decoding Complete!',
                    'content': '''
✅ **Your secret file has been successfully extracted!**

The hidden data has been recovered from the audio file. Perfect extraction complete!
'''
                }
            ]
    
    return []


def display_educational_clickthrough(file_type, mode, file_bytes, filename, mime_type):
    """Display educational click-through interface."""
    steps = get_educational_steps(file_type, mode)
    
    if st.session_state.edu_step < len(steps):
        step = steps[st.session_state.edu_step]
        
        st.success(f"✅ {mode.capitalize()} successful!")
        st.markdown(f"### {step['title']}")
        st.markdown(step['content'])
        
        col1, col2 = st.columns([1, 4])
        with col1:
            if st.button("Next →", type="primary", key=f"next_{st.session_state.edu_step}"):
                st.session_state.edu_step += 1
                st.rerun()
        with col2:
            st.write(f"Step {st.session_state.edu_step + 1} of {len(steps)}")
    else:
        # All steps completed
        st.success(f"✅ {mode.capitalize()} successful!")
        st.balloons()
        
        # Mark as completed
        st.session_state.edu_completed[file_type] = True
        
        custom_filename = st.text_input(
            "Output filename:",
            value=filename,
            key=f"{mode}_filename_{file_type}"
        )
        
        st.download_button(
            f"Download {mode.capitalize()}d File",
            data=file_bytes,
            file_name=custom_filename,
            mime=mime_type,
            type="primary"
        )
        
        # Reset for next time
        if st.button("Done"):
            st.session_state.edu_step = 0
            st.session_state.pending_download = None
            st.rerun()


def display_file_preview(uploaded_file):
    """Display preview of uploaded file."""
    if uploaded_file is None:
        return

    file_ext = Path(uploaded_file.name).suffix.lower()

    if file_ext in ['.png', '.jpg', '.jpeg']:
        st.image(uploaded_file, caption=uploaded_file.name, use_container_width=True)
    elif file_ext == '.wav':
        st.audio(uploaded_file, format='audio/wav')
    elif file_ext == '.txt':
        try:
            uploaded_file.seek(0)
            content = uploaded_file.read().decode('utf-8')
            st.code(content, language='text')
            uploaded_file.seek(0)
        except:
            st.info("Could not preview text file")
    else:
        st.info("Preview not available for this file type")


def call_backend_encode(carrier_path, secret_path, output_base, tmpdir):
    """Call C++ backend for encoding."""
    try:
        if IS_WINDOWS:
            tmpdir_wsl = windows_to_wsl_path(tmpdir)
            carrier_name = Path(carrier_path).name
            secret_name = Path(secret_path).name
            
            cmd = ['wsl', 'bash', '-c', 
                   f'cd "{tmpdir_wsl}" && ./stegasaur encode "{carrier_name}" "{secret_name}" "{output_base}"']
        else:
            cmd = [str(Path(tmpdir) / 'stegasaur'), 'encode', 
                   str(carrier_path), str(secret_path), output_base]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            cwd=None if IS_WINDOWS else tmpdir
        )

        if result.returncode == 0:
            output_text = result.stdout.strip()
            
            import re
            match = re.search(r'(\S+\.(?:png|jpg|jpeg|txt|wav))(?:\s|$)', output_text, re.IGNORECASE)
            if match:
                output_filename = match.group(1)
            else:
                output_filename = output_text.split()[-1] if output_text else "output.png"
            
            if '/' in output_filename:
                output_filename = output_filename.split('/')[-1]
            
            return True, output_filename, None
        else:
            return False, None, result.stderr

    except subprocess.TimeoutExpired:
        return False, None, "Operation timed out"
    except Exception as e:
        return False, None, f"Error: {str(e)}"


def call_backend_decode(encoded_path, output_base, tmpdir):
    """Call C++ backend for decoding."""
    try:
        if IS_WINDOWS:
            tmpdir_wsl = windows_to_wsl_path(tmpdir)
            encoded_name = Path(encoded_path).name
            
            cmd = ['wsl', 'bash', '-c', 
                   f'cd "{tmpdir_wsl}" && ./stegasaur decode "{encoded_name}" "{output_base}"']
        else:
            cmd = [str(Path(tmpdir) / 'stegasaur'), 'decode', 
                   str(encoded_path), output_base]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            cwd=None if IS_WINDOWS else tmpdir
        )
        
        if result.returncode == 0:
            output_text = result.stdout.strip()
            
            import re
            match = re.search(r'(\S+\.(?:png|jpg|jpeg|txt|wav))(?:\s|$)', output_text, re.IGNORECASE)
            if match:
                output_filename = match.group(1)
            else:
                output_filename = output_text.split()[-1] if output_text else "secret.txt"
            
            if '/' in output_filename:
                output_filename = output_filename.split('/')[-1]
            
            return True, output_filename, None
        else:
            return False, None, result.stderr
            
    except subprocess.TimeoutExpired:
        return False, None, "Operation timed out"
    except Exception as e:
        return False, None, f"Error: {str(e)}"


# Check if there's a pending educational click-through
if st.session_state.pending_download is not None:
    file_type = st.session_state.edu_file_type
    mode_type = st.session_state.edu_mode
    pending = st.session_state.pending_download
    
    display_educational_clickthrough(
        file_type, 
        mode_type, 
        pending['bytes'], 
        pending['filename'], 
        pending['mime']
    )
    st.stop()  # Don't show the rest of the UI

# Main UI
mode = st.radio("Choose Mode", ["Encode", "Decode"], horizontal=True)

if mode == "Encode":
    st.header("Encode Secret into Carrier")

    carrier_file = st.file_uploader(
        "Select Carrier File (PNG, JPEG, JPG, WAV)",
        type=['png', 'jpg', 'jpeg', 'wav'],
        key="carrier_encode"
    )

    if carrier_file:
        display_file_preview(carrier_file)

    # Determine secret file types based on carrier
    if carrier_file:
        file_ext = Path(carrier_file.name).suffix.lower()
        if file_ext == '.png':
            secret_file = st.file_uploader(
                "Select Secret File (text or image)",
                type=['txt', 'png', 'jpg', 'jpeg'],
                key="secret"
            )
        elif file_ext in ['.jpg', '.jpeg']:
            secret_file = st.file_uploader(
                "Select Secret File (text only for JPEG)",
                type=['txt'],
                key="secret"
            )
        elif file_ext == '.wav':
            secret_file = st.file_uploader(
                "Select Secret File (text or image)",
                type=['txt', 'png', 'jpg', 'jpeg'],
                key="secret"
            )
        else:
            secret_file = None
    else:
        secret_file = None

    if secret_file:
        display_file_preview(secret_file)
    
    if carrier_file and secret_file:
        st.markdown("---")
        display_capacity_info(carrier_file, secret_file)
    elif carrier_file:
        st.markdown("---")
        display_capacity_info(carrier_file)
    
    if st.button("Encode", type="primary"):
        if carrier_file and secret_file:
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                
                # Save files
                carrier_file.seek(0)
                carrier_path = tmpdir_path / f"carrier{Path(carrier_file.name).suffix}"
                with open(carrier_path, 'wb') as f:
                    f.write(carrier_file.read())
                
                secret_file.seek(0)
                secret_path = tmpdir_path / f"secret{Path(secret_file.name).suffix}"
                with open(secret_path, 'wb') as f:
                    f.write(secret_file.read())
                
                # Copy stegasaur binary
                binary_source = Path('stegasaur')
                if binary_source.exists():
                    import shutil
                    shutil.copy(binary_source, tmpdir_path / 'stegasaur')
                else:
                    st.error("stegasaur binary not found. Please run setup.bat first.")
                    st.stop()
                
                with st.spinner('Encoding...'):
                    success, output_filename, stderr = call_backend_encode(
                        carrier_path, secret_path, "output", tmpdir
                    )
                
                if success:
                    output_path = tmpdir_path / output_filename
                    
                    # Check alternate extensions
                    if not output_path.exists() and output_filename.endswith('.jpeg'):
                        alt_filename = output_filename.replace('.jpeg', '.jpg')
                        alt_path = tmpdir_path / alt_filename
                        if alt_path.exists():
                            output_path = alt_path
                            output_filename = alt_filename
                    elif not output_path.exists() and output_filename.endswith('.jpg'):
                        alt_filename = output_filename.replace('.jpg', '.jpeg')
                        alt_path = tmpdir_path / alt_filename
                        if alt_path.exists():
                            output_path = alt_path
                            output_filename = alt_filename
                    
                    if output_path.exists():
                        with open(output_path, 'rb') as f:
                            file_bytes = f.read()
                        
                        # Determine file type for educational content
                        carrier_ext = Path(carrier_file.name).suffix.lower()
                        if carrier_ext == '.png':
                            file_type = 'png'
                        elif carrier_ext in ['.jpg', '.jpeg']:
                            file_type = 'jpeg'
                        elif carrier_ext == '.wav':
                            file_type = 'wav'
                        else:
                            file_type = 'png'
                        
                        # Determine MIME type
                        if carrier_ext == '.png':
                            mime_type = "image/png"
                        elif carrier_ext in ['.jpg', '.jpeg']:
                            mime_type = "image/jpeg"
                        elif carrier_ext == '.wav':
                            mime_type = "audio/wav"
                        else:
                            mime_type = "application/octet-stream"
                        
                        default_filename = f"encoded{carrier_ext}"
                        
                        # Check if first time for this file type
                        if not st.session_state.edu_completed[file_type]:
                            # First time - mandatory click-through
                            st.session_state.edu_mode = 'encode'
                            st.session_state.edu_file_type = file_type
                            st.session_state.edu_step = 0  # Reset step counter
                            st.session_state.pending_download = {
                                'bytes': file_bytes,
                                'filename': default_filename,
                                'mime': mime_type
                            }
                            st.rerun()  # Rerun to show educational content
                        else:
                            # Not first time - optional review
                            st.success("✅ Encoding successful!")
                            
                            with st.expander("📚 Review: How it works (optional)"):
                                steps = get_educational_steps(file_type, 'encode')
                                for i, step in enumerate(steps):
                                    st.markdown(f"**{step['title']}**")
                                    st.markdown(step['content'])
                                    if i < len(steps) - 1:
                                        st.markdown("---")
                            
                            custom_filename = st.text_input(
                                "Output filename:",
                                value=default_filename,
                                key="encode_filename"
                            )
                            
                            st.download_button(
                                "Download Encoded File",
                                data=file_bytes,
                                file_name=custom_filename,
                                mime=mime_type,
                                type="primary"
                            )
                    else:
                        st.error(f"Output file not found: {output_filename}")
                else:
                    st.error("❌ Encoding failed")
                    with st.expander("Show Details"):
                        st.code(stderr, language='text')
        else:
            st.warning("Please select both carrier and secret files")


else:  # Decode
    st.header("Decode Secret from Carrier")

    encoded_file = st.file_uploader(
        "Select Encoded File (PNG, JPEG, JPG, WAV)",
        type=['png', 'jpg', 'jpeg', 'wav'],
        key="carrier_decode"
    )

    if encoded_file:
        display_file_preview(encoded_file)
    
    if st.button("Decode", type="primary"):
        if encoded_file:
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                
                # Save file
                encoded_file.seek(0)
                encoded_path = tmpdir_path / f"encoded{Path(encoded_file.name).suffix}"
                with open(encoded_path, 'wb') as f:
                    f.write(encoded_file.read())
                
                # Copy stegasaur binary
                binary_source = Path('stegasaur')
                if binary_source.exists():
                    import shutil
                    shutil.copy(binary_source, tmpdir_path / 'stegasaur')
                else:
                    st.error("stegasaur binary not found. Please run setup.bat first.")
                    st.stop()
                
                with st.spinner('Decoding...'):
                    success, output_filename, stderr = call_backend_decode(
                        encoded_path, "secret", tmpdir
                    )
                
                if success:
                    # Try to find the output file
                    output_path = None
                    for ext in ['.txt', '.png', '.jpg', '.jpeg', '.wav']:
                        test_path = tmpdir_path / f"secret{ext}"
                        if test_path.exists():
                            output_path = test_path
                            break
                    
                    if output_path and output_path.exists():
                        with open(output_path, 'rb') as f:
                            file_bytes = f.read()
                        
                        # Determine file type for educational content
                        carrier_ext = Path(encoded_file.name).suffix.lower()
                        if carrier_ext == '.png':
                            file_type = 'png'
                        elif carrier_ext in ['.jpg', '.jpeg']:
                            file_type = 'jpeg'
                        elif carrier_ext == '.wav':
                            file_type = 'wav'
                        else:
                            file_type = 'png'
                        
                        file_ext = output_path.suffix.lower()
                        default_filename = f"secret{file_ext}"
                        
                        # Determine MIME type
                        if file_ext == '.png':
                            mime_type = "image/png"
                        elif file_ext in ['.jpg', '.jpeg']:
                            mime_type = "image/jpeg"
                        elif file_ext == '.wav':
                            mime_type = "audio/wav"
                        elif file_ext == '.txt':
                            mime_type = "text/plain"
                        else:
                            mime_type = "application/octet-stream"
                        
                        # Check if first time for this file type
                        if not st.session_state.edu_completed[file_type]:
                            # First time - mandatory click-through
                            st.session_state.edu_mode = 'decode'
                            st.session_state.edu_file_type = file_type
                            st.session_state.edu_step = 0  # Reset step counter
                            st.session_state.pending_download = {
                                'bytes': file_bytes,
                                'filename': default_filename,
                                'mime': mime_type
                            }
                            st.rerun()  # Rerun to show educational content
                        else:
                            # Not first time - optional review
                            st.success("✅ Decoding successful!")
                            
                            with st.expander("📚 Review: How it works (optional)"):
                                steps = get_educational_steps(file_type, 'decode')
                                for i, step in enumerate(steps):
                                    st.markdown(f"**{step['title']}**")
                                    st.markdown(step['content'])
                                    if i < len(steps) - 1:
                                        st.markdown("---")
                            
                            custom_filename = st.text_input(
                                "Output filename:",
                                value=default_filename,
                                key="decode_filename"
                            )
                            
                            st.download_button(
                                "Download Decoded File",
                                data=file_bytes,
                                file_name=custom_filename,
                                mime=mime_type,
                                type="primary"
                            )
                    else:
                        st.error("Output file not found")
                else:
                    st.error("❌ Decoding failed")
                    with st.expander("Show Details"):
                        st.code(stderr, language='text')
        else:
            st.warning("Please select an encoded file")

st.markdown("---")
st.markdown("**Complete Version** - Full PNG, JPEG, and WAV support with educational click-through")
