import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import Logo from '../components/Logo'
import BackButton from '../components/BackButton'

function Decode() {
  const navigate = useNavigate()
  const [carrierFile, setCarrierFile] = useState(null)
  const [isLoading, setIsLoading] = useState(false)
  const [error, setError] = useState('')

  const handleCarrierChange = (e) => {
    const file = e.target.files[0]
    if (file) {
      if (file.name.endsWith('.png')) {
        setCarrierFile(file)
        setError('')
      } else {
        setError('Encoded file must be PNG')
      }
    }
  }

  const handleDecode = async () => {
    if (!carrierFile) {
      setError('Please select an encoded file')
      return
    }

    setIsLoading(true)
    setError('')

    const formData = new FormData()
    formData.append('encoded', carrierFile)

    try {
      const response = await fetch('/api/decode', {
        method: 'POST',
        body: formData,
      })

      if (response.ok) {
        const blob = await response.blob()
        const url = window.URL.createObjectURL(blob)
        const contentDisposition = response.headers.get('Content-Disposition')
        let fileName = 'decoded.txt'
        if (contentDisposition) {
          const match = contentDisposition.match(/filename[^;=\n]*=((['"]).*?\2|[^;\n]*)/)
          if (match && match[1]) {
            fileName = match[1].replace(/['"]/g, '')
          }
        }
        navigate('/decode-result', { 
          state: { 
            fileUrl: url, 
            fileName: fileName,
            carrierName: carrierFile.name
          } 
        })
      } else {
        const data = await response.json()
        setError(data.error || 'Decoding failed')
      }
    } catch (err) {
      setError('Network error: ' + err.message)
    } finally {
      setIsLoading(false)
    }
  }

  return (
    <div className="container">
      <BackButton />
      <Logo />
      
      <div className="form-group">
        <label className="label">Carrier</label>
        <input
          type="file"
          id="carrier-input"
          accept=".png"
          onChange={handleCarrierChange}
        />
        <label htmlFor="carrier-input" className={`file-input ${carrierFile ? 'filled' : ''}`}>
          {carrierFile ? carrierFile.name : 'CarrierFile.*ext'}
        </label>
      </div>

      <div className="button-group">
        <button 
          className="button outline" 
          onClick={() => navigate('/encode')}
        >
          Encode
        </button>
        <button 
          className="button primary" 
          onClick={handleDecode}
          disabled={isLoading || !carrierFile}
        >
          {isLoading ? 'Decoding...' : 'Decode'}
        </button>
      </div>

      {error && <div className="error">{error}</div>}
    </div>
  )
}

export default Decode
