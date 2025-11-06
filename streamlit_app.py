import streamlit as st
import subprocess
import tempfile
import os
from pathlib import Path
from PIL import Image
import io

st.set_page_config(page_title="stegaSaur MVP", page_icon="🦕", layout="centered")

st.title("🦕 stegaSaur")
st.subheader("Steganography Educational Tool")

#determine if running on Windows and use WSL path
IS_WINDOWS = os.name == 'nt'
STEGASAUR_BINARY = './stegasaur' if not IS_WINDOWS else 'wsl ./stegasaur'


def windows_to_wsl_path(windows_path):
    """Convert Windows path to WSL path format."""
    if not IS_WINDOWS:
        return windows_path
    
    #convert Path object to string
    path_str = str(windows_path)
    
    #replace backslashes with forward slashes
    path_str = path_str.replace('\\', '/')
    
    #convert drive letter (e.g., C: -> /mnt/c)
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
            pixel_bytes = width * height * 4  # RGBA
            capacity_bytes = (pixel_bytes // 8) - 29  # Account for metadata
            
            return max(0, capacity_bytes), {
                'width': width,
                'height': height,
                'type': 'PNG (LSB)'
            }
            
        elif file_ext in ['.jpg', '.jpeg']:
            file_size = len(file_data)
            capacity_bytes = int(file_size * 0.10) - 25  # Conservative estimate
            
            return max(0, capacity_bytes), {
                'file_size': file_size,
                'type': 'JPEG (DCT)',
                'note': 'Estimated capacity'
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


def get_educational_content(file_ext, mode):
    """Generate educational content based on file type and mode."""
    file_ext = file_ext.lower()

    if mode == "encode":
        if file_ext == '.png':
            return """
**LSB (Least Significant Bit) Steganography**

This technique hides data by modifying the least significant bit of each pixel's color values.

**How it works:**
- Each pixel has 4 bytes (Red, Green, Blue, Alpha)
- We modify the last bit of each byte to store 1 bit of secret data
- Changes are imperceptible to the human eye

**Example:** RGB(255, 128, 64) → RGB(254, 129, 64)
"""
        elif file_ext in ['.jpeg', '.jpg']:
            return """
**DCT (Discrete Cosine Transform) Steganography**

JPEG images use DCT compression. This technique hides data by modifying DCT coefficients.

**How it works:**
- JPEG divides images into 8×8 pixel blocks
- Each block has 64 DCT coefficients
- We modify the LSB of non-zero, non-one coefficients
- Preserves image quality while hiding data

**Note:** Lower capacity than PNG, best for text files
"""
    else:  # decode mode
        if file_ext == '.png':
            return """
**LSB Extraction**

The decoder reads the least significant bit from each byte and reconstructs the original file.

**Process:**
1. Extract metadata (file type, size)
2. Verify checksum for data integrity
3. Extract and reconstruct the secret file
"""
        elif file_ext in ['.jpeg', '.jpg']:
            return """
**DCT Extraction**

The decoder reads DCT coefficients and extracts hidden bits.

**Process:**
1. Read JPEG DCT coefficients
2. Extract LSB from valid coefficients
3. Verify checksum and reconstruct file
"""

    return None


def display_file_preview(uploaded_file):
    """Display preview of uploaded file."""
    if uploaded_file is None:
        return

    file_ext = Path(uploaded_file.name).suffix.lower()

    if file_ext in ['.png', '.jpg', '.jpeg']:
        st.image(uploaded_file, caption=uploaded_file.name, use_container_width=True)
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
    #convert paths for WSL if on Windows
        if IS_WINDOWS:
            tmpdir_wsl = windows_to_wsl_path(tmpdir)
            #use just filenames since we're cd'ing into the directory
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
            #the backend outputs console messages mixed with the filename
            #extract just the filename (last word that ends with an extension)
            output_text = result.stdout.strip()
            
            #look for a filename pattern (ends with .png, .jpg, .jpeg, etc.)
            import re
            match = re.search(r'(\S+\.(?:png|jpg|jpeg|txt))(?:\s|$)', output_text, re.IGNORECASE)
            if match:
                output_filename = match.group(1)
            else:
                #fallback: take the last word
                output_filename = output_text.split()[-1] if output_text else "output.png"
            
            #extract just the filename if it's a full path
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
    #convert paths for WSL if on Windows
        if IS_WINDOWS:
            tmpdir_wsl = windows_to_wsl_path(tmpdir)
            #use just filename since we're cd'ing into the directory
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
            #the backend outputs console messages mixed with the filename
            #extract just the filename (last word that ends with an extension)
            output_text = result.stdout.strip()
            
            #look for a filename pattern (ends with .png, .jpg, .jpeg, .txt, etc.)
            import re
            match = re.search(r'(\S+\.(png|jpg|jpeg|txt))(?:\s|$)', output_text, re.IGNORECASE)
            if match:
                output_filename = match.group(1)
            else:
                #fallback: take the last word
                output_filename = output_text.split()[-1] if output_text else "secret.txt"
            
            #extract just the filename if it's a full path
            if '/' in output_filename:
                output_filename = output_filename.split('/')[-1]
            
            return True, output_filename, None
        else:
            return False, None, result.stderr
            
    except subprocess.TimeoutExpired:
        return False, None, "Operation timed out"
    except Exception as e:
        return False, None, f"Error: {str(e)}"


#main UI
mode = st.radio("Choose Mode", ["Encode", "Decode"], horizontal=True)

if mode == "Encode":
    st.header("Encode Secret into Carrier")

    carrier_file = st.file_uploader(
        "Select Carrier File (PNG, JPEG, JPG)",
        type=['png', 'jpg', 'jpeg'],
        key="carrier_encode"
    )

    if carrier_file:
        display_file_preview(carrier_file)
        file_ext = Path(carrier_file.name).suffix.lower()
        educational_content = get_educational_content(file_ext, "encode")
        if educational_content:
            st.info(educational_content)

    #for mvp: png accepts text and images, jpeg only accepts text
    if carrier_file:
        file_ext = Path(carrier_file.name).suffix.lower()
        if file_ext == '.png':
            secret_file = st.file_uploader(
                "Select Secret File (text or image)",
                type=['txt', 'png', 'jpg', 'jpeg'],
                key="secret"
            )
        else:  # JPEG
            secret_file = st.file_uploader(
                "Select Secret File (text only for JPEG)",
                type=['txt'],
                key="secret"
            )
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
                
                #save files
                carrier_file.seek(0)
                carrier_path = tmpdir_path / f"carrier{Path(carrier_file.name).suffix}"
                with open(carrier_path, 'wb') as f:
                    f.write(carrier_file.read())
                
                secret_file.seek(0)
                secret_path = tmpdir_path / f"secret{Path(secret_file.name).suffix}"
                with open(secret_path, 'wb') as f:
                    f.write(secret_file.read())
                
                #copy stegasaur binary to temp directory
                #always use 'stegasaur' (WSL-compiled binary)
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
                    
                    #check if file exists, if not try alternate JPEG extension
                    if not output_path.exists() and output_filename.endswith('.jpeg'):
                        #try .jpg instead
                        alt_filename = output_filename.replace('.jpeg', '.jpg')
                        alt_path = tmpdir_path / alt_filename
                        if alt_path.exists():
                            output_path = alt_path
                            output_filename = alt_filename
                    elif not output_path.exists() and output_filename.endswith('.jpg'):
                        #try .jpeg instead
                        alt_filename = output_filename.replace('.jpg', '.jpeg')
                        alt_path = tmpdir_path / alt_filename
                        if alt_path.exists():
                            output_path = alt_path
                            output_filename = alt_filename
                    
                    if output_path.exists():
                        with open(output_path, 'rb') as f:
                            file_bytes = f.read()
                        
                        st.success("✅ Encoding successful!")
                        
                        carrier_ext = Path(carrier_file.name).suffix.lower()
                        default_filename = f"encoded{carrier_ext}"
                        
                        custom_filename = st.text_input(
                            "Output filename:",
                            value=default_filename,
                            key="encode_filename"
                        )
                        
                        st.download_button(
                            "Download Encoded File",
                            data=file_bytes,
                            file_name=custom_filename,
                            mime="image/png" if carrier_ext == '.png' else "image/jpeg"
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
        "Select Encoded File (PNG, JPEG, JPG)",
        type=['png', 'jpg', 'jpeg'],
        key="carrier_decode"
    )

    if encoded_file:
        display_file_preview(encoded_file)
        file_ext = Path(encoded_file.name).suffix.lower()
        educational_content = get_educational_content(file_ext, "decode")
        if educational_content:
            st.info(educational_content)
    
    if st.button("Decode", type="primary"):
        if encoded_file:
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                
                #save file
                encoded_file.seek(0)
                encoded_path = tmpdir_path / f"encoded{Path(encoded_file.name).suffix}"
                with open(encoded_path, 'wb') as f:
                    f.write(encoded_file.read())
                
                #copy stegasaur binary to temp directory
                #always use 'stegasaur' (WSL-compiled binary)
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
                    #try to find the output file
                    output_path = None
                    for ext in ['.txt', '.png', '.jpg', '.jpeg']:
                        test_path = tmpdir_path / f"secret{ext}"
                        if test_path.exists():
                            output_path = test_path
                            break
                    
                    if output_path and output_path.exists():
                        with open(output_path, 'rb') as f:
                            file_bytes = f.read()
                        
                        st.success("✅ Decoding successful!")
                        
                        file_ext = output_path.suffix.lower()
                        default_filename = f"secret{file_ext}"
                        
                        custom_filename = st.text_input(
                            "Output filename:",
                            value=default_filename,
                            key="decode_filename"
                        )
                        
                        mime_type = "application/octet-stream"
                        if file_ext == '.png':
                            mime_type = "image/png"
                        elif file_ext in ['.jpg', '.jpeg']:
                            mime_type = "image/jpeg"
                        elif file_ext == '.txt':
                            mime_type = "text/plain"
                        
                        st.download_button(
                            "Download Decoded File",
                            data=file_bytes,
                            file_name=custom_filename,
                            mime=mime_type
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
st.markdown("**MVP Demo Version** - PNG: text & images | JPEG: text only")
